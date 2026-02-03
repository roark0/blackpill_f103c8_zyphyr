#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/arch/cpu.h>
#include "ds18b20.h"
#include "led.h"
#include "switch.h"
#include "display.h"
#include "pid.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// GPIO 设备节点
#define HEATER_NODE DT_ALIAS(heater)

// GPIO 设备结构
static const struct gpio_dt_spec heater = GPIO_DT_SPEC_GET(HEATER_NODE, gpios);

// 主函数
int main(void)
{
    int ret;

    LOG_INF("Temperature Control System Starting...");

    // 初始化 LED
    ret = led_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize LEDs");
        return ret;
    }
#if 1
    // 初始化按键中断
    ret = button_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize button interrupt");
        return ret;
    }

    // 初始化开关
    ret = switch_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize switches");
        return ret;
    }

    // 初始化 DS18B20
    if (ds18b20_init() != 0)
    {
        LOG_ERR("Failed to initialize DS18B20");
        return -1;
    }

    if (!device_is_ready(heater.port))
    {
        LOG_ERR("Heater device not ready");
        return -1;
    }

    // 配置 GPIO
    ret = gpio_pin_configure_dt(&heater, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure heater");
        return ret;
    }
#endif
    // 初始化 LED 状态
    led_set_single(2);  // 默认点亮第 3 个 LED（索引 2）
    gpio_pin_set_dt(&heater, 0);

    // 初始化数码管
    ret = display_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize display");
        return ret;
    }

    LOG_INF("System initialized, starting main loop");
    uint8_t temp_idx       = 2;
    float temp_offset      = 0.0f;
    float temp_setpoints[] = {25.0f, 30.0f, 37.0f, 45.0f};
    float temp;
    float disp_temp;
    float last_disp_temp = 37;

    // 初始化 PID 控制器
    // Kp=0.8, Ki=0.05, Kd=0.3, 输出范围 0-1
    // 降低 Kp 防止温度过载，增加 Ki 提高稳态精度
    pid_controller_t pid;
    pid_init(&pid, 1.0f, 0.05f, 0.3f, temp_setpoints[temp_idx], 0.0f, 1.0f);

    // 主循环
    while (1)
    {
        // 读取开关设置并直接应用校准（包含 SET_S 方向控制）
        temp_offset = switch_read_settings();  //  + 1.0f
        LOG_DBG("temp_offset=%.3f", (double)temp_offset);
        temp = ds18b20_read_temperature(temp_offset);
        LOG_INF("temp=%.2f, %.2f", (double)temp, (double)temp_offset);

        float setpoint = temp_setpoints[temp_idx];

        // 更新 PID 设定值
        pid_set_setpoint(&pid, setpoint);

        // 计算 PID 输出
        float pid_output = pid_compute(&pid, temp);

        // PID 输出 > 0.3 时开启加热器（阈值可调）
        if (pid_output > 0.15f && temp > 0.0f)
        {
            gpio_pin_set_dt(&heater, 1);
            LOG_WRN("heater");
        }
        else
        {
            // LOG_INF("no heater");
            gpio_pin_set_dt(&heater, 0);
        }

#define TARGET_TIMES 0
#define LAST_TIMES 10

        disp_temp      = (last_disp_temp * LAST_TIMES + temp + setpoint * TARGET_TIMES) / (TARGET_TIMES + LAST_TIMES + 1);
        last_disp_temp = disp_temp;

        // 显示温度
        // 温度控制逻辑（使用 PID 输出阈值控制）
        LOG_INF("display_temperature=%.2f, setpoint=%.2f, pid_output=%.2f", (double)disp_temp, (double)pid.setpoint, (double)pid_output);

        display_temp(disp_temp);

        // 处理按键事件（中断方式）
        if (button_is_pressed())
        {
            button_clear_pressed();  // 清除按键标志

            gpio_pin_set_dt(&heater, 0);  // 关闭加热器

            // 循环切换温度档位
            temp_idx = (temp_idx + 1) % 4;

            // 设置 LED 状态：只有当前活动的 LED 亮起
            led_set_single(temp_idx);

            // 重置 PID 控制器
            pid_reset(&pid);
            setpoint = temp_setpoints[temp_idx];
            pid_set_setpoint(&pid, setpoint);
        }

        k_msleep(300);  // 主循环延时
        gpio_pin_set_dt(&heater, 0);
        if (setpoint - temp > 3)
        {
            LOG_WRN("header 100, %.2f, %.2f", (double)temp, (double)setpoint);
            gpio_pin_set_dt(&heater, 1);
        }
        k_msleep(700);
    }

    return 0;
}