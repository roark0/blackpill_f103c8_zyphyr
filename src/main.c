#include "zephyr/sys/printk.h"
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
    float current_temperature;
    float display_temperature;
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
    ret = gpio_pin_configure_dt(&heater, GPIO_OUTPUT| GPIO_OPEN_DRAIN);
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
    uint8_t current_temperature_index     = 2;
    float temperature_offset              = 0.0f;                          // 温度偏移值
    float base_temperature_setpoints[]    = {25.3f, 30.3f, 37.3f, 45.3f};  // 温度设置数组
    float current_temperature_setpoints[] = {25.3f, 30.3f, 37.3f, 45.3f};

    // 初始化 PID 控制器
    // Kp=0.8, Ki=0.05, Kd=0.3, 输出范围 0-1
    // 降低 Kp 防止温度过载，增加 Ki 提高稳态精度
    pid_controller_t pid;
    pid_init(&pid, 0.8f, 0.05f, 0.3f, current_temperature_setpoints[current_temperature_index], 0.0f, 1.0f);

    // 主循环
    while (1)
    {
        // 读取开关设置并直接应用校准（包含 SET_S 方向控制）
        temperature_offset                                       = switch_read_settings();
        current_temperature_setpoints[current_temperature_index] = base_temperature_setpoints[current_temperature_index] + temperature_offset;

        current_temperature = ds18b20_read_temperature();

        // 更新 PID 设定值
        pid_set_setpoint(&pid, current_temperature_setpoints[current_temperature_index]);

        // 计算 PID 输出
        float pid_output = pid_compute(&pid, current_temperature);

        // 温度控制逻辑（使用 PID 输出阈值控制）
        LOG_INF("current_temperature=%.2f, setpoint=%.2f, pid_output=%.2f", (double)current_temperature,
                (double)pid.setpoint, (double)pid_output);

        // PID 输出 > 0.3 时开启加热器（阈值可调）
        if (pid_output > 0.4f && current_temperature > 0.0f)
        {
            gpio_pin_set_dt(&heater, 1);
            led_set_state(3, 1);
            LOG_WRN("heater");
        }
        else
        {
            LOG_INF("no heater");
            gpio_pin_set_dt(&heater, 0);
            led_set_state(3, 0);
        }
      
        // 计算输出温度（添加固定的 -4 偏移，对应 fugaijin.c 的 temp - 4）
        display_temperature = current_temperature - 0.4f + temperature_offset;

        // 显示温度
        display_temp(display_temperature);

        // 处理按键事件（中断方式）
        if (button_is_pressed())
        {
            button_clear_pressed();  // 清除按键标志

            gpio_pin_set_dt(&heater, 0);  // 关闭加热器

            // 循环切换温度档位
            current_temperature_index = (current_temperature_index + 1) % 4;

            // 设置 LED 状态：只有当前活动的 LED 亮起
            led_set_single(current_temperature_index);

            // 重置 PID 控制器
            pid_reset(&pid);
            pid_set_setpoint(&pid, current_temperature_setpoints[current_temperature_index]);
        }

        k_msleep(100);  // 主循环延时
    }

    return 0;
}