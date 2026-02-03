/**
 * @file led.c
 * @brief LED 控制模块实现
 * 
 * 该文件实现了 LED 控制功能，包括初始化、单个控制、批量控制和模式控制。
 */

#include "led.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);

//!< LED GPIO 设备定义
static const struct gpio_dt_spec leds[LED_COUNT] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

/**
 * @brief LED 初始化函数
 * 
 * 初始化所有 LED 引脚为输出模式，并将所有 LED 设置为熄灭状态。
 * 
 * @return int 成功返回 0，失败返回负值
 */
int led_init(void)
{
    int ret;

    // 检查所有 LED 设备是否就绪
    for (int i = 0; i < LED_COUNT; i++)
    {
        if (!device_is_ready(leds[i].port))
        {
            LOG_ERR("LED%d device not ready", i);
            return -1;
        }
    }

    // 配置所有 LED 为输出
    for (int i = 0; i < LED_COUNT; i++)
    {
        ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);
        if (ret != 0)
        {
            LOG_ERR("Failed to configure LED%d", i);
            return ret;
        }
    }

    // 初始化所有 LED 为熄灭状态
    led_set_all(0);

    LOG_INF("led_set_all");

    return 0;
}

/**
 * @brief 设置单个 LED 状态
 * 
 * 设置指定索引的 LED 为指定状态（亮或灭）。
 * 
 * @param led_index LED 索引 (0-3)
 * @param state LED 状态 (0=灭, 1=亮)
 */
void led_set_state(uint8_t led_index, int state)
{
    if (led_index >= LED_COUNT)
    {
        LOG_ERR("LED index out of range: %d", led_index);
        return;
    }

    gpio_pin_set_dt(&leds[led_index], state);
}

/**
 * @brief 设置所有 LED 状态
 * 
 * 将所有 LED 设置为相同的状态（全部亮或全部灭）。
 * 
 * @param state LED 状态 (0=全灭, 1=全亮)
 */
void led_set_all(int state)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        gpio_pin_set_dt(&leds[i], state);
    }
}

/**
 * @brief 只亮一个 LED，其他全部熄灭
 * 
 * 点亮指定索引的 LED，同时熄灭其他所有 LED。
 * 
 * @param led_index 要点亮的 LED 索引 (0-3)
 */
void led_set_single(uint8_t led_index)
{
    if (led_index >= LED_COUNT)
    {
        LOG_ERR("LED index out of range: %d", led_index);
        return;
    }

    for (int i = 0; i < LED_COUNT; i++)
    {
        gpio_pin_set_dt(&leds[i], (i == led_index) ? 1 : 0);
    }
}

/**
 * @brief 设置 LED 模式（使用位掩码）
 * 
 * 根据位掩码设置 LED 的状态，每一位对应一个 LED。
 * 
 * @param pattern 位掩码模式，每一位表示对应 LED 的状态
 */
void led_set_pattern(uint8_t pattern)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        int state = (pattern & (1 << i)) ? 1 : 0;
        gpio_pin_set_dt(&leds[i], state);
    }
}