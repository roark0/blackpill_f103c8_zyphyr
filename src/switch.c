/**
 * @file switch.c
 * @brief 开关和按键控制模块实现
 * 
 * 该文件实现了拨码开关和按键的控制功能，包括初始化、读取开关设置、
 * 按键中断处理等。按键中断采用工作队列实现，避免在中断上下文中执行复杂操作。
 */

#include "switch.h"
#include "zephyr/devicetree.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bsp.h"
#include "temperature_control.h"

LOG_MODULE_REGISTER(switch, LOG_LEVEL_INF);

//!< 档位切换按钮 GPIO 设备结构
static const struct gpio_dt_spec mode_btn = GPIO_DT_SPEC_GET(DT_NODELABEL(mode_btn), gpios);
//!< 拨码开关 GPIO 设备结构数组
static const struct gpio_dt_spec switches[SWITCH_COUNT] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios),  //!< SW1: 控制偏移方向
    GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios),  //!< SW3: 二进制位 2 (值 8)
    GPIO_DT_SPEC_GET(DT_ALIAS(sw5), gpios),  //!< SW5: 二进制位 1 (值 4)
    GPIO_DT_SPEC_GET(DT_ALIAS(sw7), gpios),  //!< SW7: 二进制位 0 (值 2)
};

//!< 按键中断回调数据结构
static struct gpio_callback button_cb_data;
//!< 按键按下标志
static volatile bool button_pressed = false;
//!< 按键工作队列项
static struct k_work button_work;

/**
 * @brief 按键工作队列处理函数
 * 
 * 在工作队列上下文中处理按键事件，包括去抖动和向温度控制 AO 发送事件。
 * 
 * @param work 指向工作队列项的指针
 */
static void button_work_handler(struct k_work *work)
{
    LOG_DBG("button_work_handler: checking button state");
    // 延时消抖
    k_sleep(K_MSEC(20));
    // 检查按键状态
    int pin_state = gpio_pin_get_dt(&mode_btn);
    if (pin_state == 0)  // 按键仍然按下（低电平）
    {
        LOG_INF("Button pressed, sending BUTTON_PRESSED_SIG to TempCtrl AO");
        button_pressed = true;  // 确认按键按下

        /* 创建按键按下事件并发送给 Temperature Control AO */
        QEvt *evt = Q_NEW(QEvt, BUTTON_PRESSED_SIG);
        if (evt != NULL) {
            /* 获取 TempCtrl AO 实例（需要从 temperature_control.c 导出） */
            extern TempCtrl l_tempCtrl;
            QACTIVE_POST(&l_tempCtrl.super, evt, 0U);
        } else {
            LOG_ERR("Failed to allocate BUTTON_PRESSED_SIG event");
        }
    } else {
        LOG_DBG("Button released (debounce)");
    }
}

/**
 * @brief 按键中断回调函数
 * 
 * GPIO 中断触发时的回调函数，将按键处理任务提交到工作队列。
 * 
 * @param dev 触发中断的设备指针
 * @param cb GPIO 回调结构体指针
 * @param pins 触发中断的引脚掩码
 */
static void button_pressed_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // 提交工作到工作队列，在中断上下文外处理
    k_work_submit(&button_work);
}

/**
 * @brief 初始化所有拨码开关
 * 
 * 配置所有拨码开关引脚为输入模式。
 * 
 * @return int 成功返回 0，失败返回负值
 */
int switch_init(void)
{
    int ret;

    // 检查所有开关设备是否就绪
    for (int i = 0; i < SWITCH_COUNT; i++)
    {
        if (!device_is_ready(switches[i].port))
        {
            LOG_ERR("SW%d device not ready", i);
            return -1;
        }
    }

    // 配置所有开关为输入
    for (int i = 0; i < SWITCH_COUNT; i++)
    {
        ret = gpio_pin_configure_dt(&switches[i], GPIO_INPUT);
        if (ret != 0)
        {
            LOG_ERR("Failed to configure SW%d", i);
            return ret;
        }
    }

    return 0;
}

/**
 * @brief 读取拨码开关设置
 * 
 * 读取拨码开关的设置值，SW3、SW5、SW7 组合输出 0-7 范围的值，
 * 再乘以 0.2 得到 0-1.4 的幅度值，并根据 SW1 (SET_S) 状态
 * 返回带符号的偏移值。
 * 
 * @return float 拨码开关设置的偏移值
 */
float switch_read_settings(void)
{
    char value = 0;
    float set_num;

    // 拨码开关二进制编码（对应 fugaijin.c 的 P2 & 0x70）：
    // SW7 -> bit 0 (值 2)
    // SW5 -> bit 1 (值 4)
    // SW3 -> bit 2 (值 8)

    // 读取 SW3, SW5, SW7 状态
    if (gpio_pin_get_dt(&switches[3]))  // SW3
    {
        value |= 0x01;  // bit 0
    }
    if (gpio_pin_get_dt(&switches[2]))  // SW5
    {
        value |= 0x02;  // bit 1
    }
    if (gpio_pin_get_dt(&switches[1]))  // SW7
    {
        value |= 0x04;  // bit 2
    }

    // 将 0-7 转换为 0-1.4
    set_num = (float)(value * 0.2);

    // 读取 SW1 (SET_S) 控制偏移方向
    int set_s_state = gpio_pin_get_dt(&switches[0]);  // SW1

    // 根据 SET_S 方向返回带符号的偏移值
    if (set_s_state == 0)  // SW1 没按下 (高电平)
    {
        return -set_num;  // 减法方向
    }
    else  // SW1 按下 (低电平)
    {
        return set_num;  // 加法方向
    }
}

/**
 * @brief 初始化按键中断
 * 
 * 配置档位切换按钮的 GPIO 中断，设置为下降沿触发，并初始化相关的工作队列。
 * 
 * @return int 成功返回 0，失败返回负值
 */
int button_init(void)
{
    int ret;

    if (!device_is_ready(mode_btn.port))
    {
        LOG_ERR("Mode button device not ready");
        return -1;
    }

    // 配置档位切换按钮引脚为输入，启用上拉
    ret = gpio_pin_configure_dt(&mode_btn, GPIO_INPUT | GPIO_PULL_UP);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure mode button");
        return ret;
    }

    // 初始化回调结构
    gpio_init_callback(&button_cb_data, button_pressed_callback, BIT(mode_btn.pin));

    // 添加回调
    ret = gpio_add_callback(mode_btn.port, &button_cb_data);
    if (ret != 0)
    {
        LOG_ERR("Failed to add button callback");
        return ret;
    }

    // 配置中断触发方式：下降沿触发（按下）
    ret = gpio_pin_interrupt_configure_dt(&mode_btn, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure button interrupt");
        return ret;
    }

    // 初始化工作队列
    k_work_init(&button_work, button_work_handler);

    LOG_INF("Button interrupt initialized");
    return 0;
}
