#include "led.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);

// LED GPIO 设备定义
static const struct gpio_dt_spec leds[LED_COUNT] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

// LED 初始化函数
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

// 设置单个 LED 状态
void led_set_state(uint8_t led_index, int state)
{
    if (led_index >= LED_COUNT)
    {
        LOG_ERR("LED index out of range: %d", led_index);
        return;
    }

    gpio_pin_set_dt(&leds[led_index], state);
}

// 设置所有 LED 状态
void led_set_all(int state)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        gpio_pin_set_dt(&leds[i], state);
    }
}

// 只亮一个 LED，其他全部熄灭
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

// 设置 LED 模式（使用位掩码）
void led_set_pattern(uint8_t pattern)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        int state = (pattern & (1 << i)) ? 1 : 0;
        gpio_pin_set_dt(&leds[i], state);
    }
}