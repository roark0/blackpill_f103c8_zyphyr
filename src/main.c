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

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// GPIO 设备节点
#define HEATER_NODE DT_ALIAS(heater)

// GPIO 设备结构
static const struct gpio_dt_spec heater = GPIO_DT_SPEC_GET(HEATER_NODE, gpios);

// 主函数
int main(void)
{
    float current_temperature;
    float target_temperature = 37.3;
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
    ret = gpio_pin_configure_dt(&heater, GPIO_OUTPUT);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure heater");
        return ret;
    }
    #endif
    // 初始化 LED 状态
    led_set_state(0, 0);
    led_set_state(1, 0);
    led_set_state(2, 1);
    led_set_state(3, 0);
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
    float previous_temperature            = 0.0f;
    uint32_t heating_counter              = 0;                             // 加热计数器
    float temperature_offset              = 0.0f;                          // 温度偏移值
    float base_temperature_setpoints[]    = {25.3f, 30.3f, 37.3f, 45.3f};  // 温度设置数组
    float current_temperature_setpoints[] = {25.3f, 30.3f, 37.3f, 45.3f};

    // 主循环
    while (1)
    {
        // 读取开关设置并直接应用校准（包含 SET_S 方向控制）
        temperature_offset                                       = switch_read_settings();
        current_temperature_setpoints[current_temperature_index] = base_temperature_setpoints[current_temperature_index] + temperature_offset;

        heating_counter++;
        current_temperature = ds18b20_read_temperature();

        if (heating_counter > 100)
        {
            if (previous_temperature < current_temperature)
            {
                target_temperature = current_temperature_setpoints[current_temperature_index] - 2.0f;
            }
            else
            {
                target_temperature = current_temperature_setpoints[current_temperature_index];
            }
            heating_counter      = 0;
            previous_temperature = current_temperature;
        }

        // 温度控制逻辑
        LOG_INF("current_temperature=%.2f, target_temperature=%.2f, %.2f", (double)current_temperature, (double)target_temperature,
                (double)current_temperature_setpoints[current_temperature_index]);
        if ((current_temperature < target_temperature) && (current_temperature > 0.0f))
        {
            gpio_pin_set_dt(&heater, 1);
            LOG_INF("heater");
        }
        else
        {
            LOG_INF("no heater");
            gpio_pin_set_dt(&heater, 0);
        }

        gpio_pin_set_dt(&heater, 1);
      
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
            for (int i = 0; i < 4; i++)
            {
                led_set_state(i, (i == current_temperature_index) ? 1 : 0);
            }
        }

        k_msleep(100);  // 主循环延时
    }

    return 0;
}