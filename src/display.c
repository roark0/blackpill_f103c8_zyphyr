#include "display.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

// 数码管段选数组定义
#define SEGMENT_COUNT 4
static const struct gpio_dt_spec tens_segments[SEGMENT_COUNT] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(tens_a), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(tens_b), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(tens_c), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(tens_d), gpios),
};
static const struct gpio_dt_spec units_segments[SEGMENT_COUNT] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(units_a), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(units_b), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(units_c), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(units_d), gpios),
};
static const struct gpio_dt_spec decimal_segments[SEGMENT_COUNT] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(decimal_a), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(decimal_b), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(decimal_c), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(decimal_d), gpios),
};

// 数码管段码表 (0-9)
static const unsigned char digit_segments[] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F   // 9
};

// 初始化数码管 GPIO
int display_init(void)
{
    int ret;

    // 配置十位数码管段选
    for (int i = 0; i < SEGMENT_COUNT; i++)
    {
        ret = gpio_pin_configure_dt(&tens_segments[i], GPIO_OUTPUT);
        if (ret != 0)
            return ret;
    }

    // 配置个位数码管段选
    for (int i = 0; i < SEGMENT_COUNT; i++)
    {
        ret = gpio_pin_configure_dt(&units_segments[i], GPIO_OUTPUT);
        if (ret != 0)
            return ret;
    }

    // 配置小数位数码管段选
    for (int i = 0; i < SEGMENT_COUNT; i++)
    {
        ret = gpio_pin_configure_dt(&decimal_segments[i], GPIO_OUTPUT);
        if (ret != 0)
            return ret;
    }

    // 初始化所有段为熄灭状态
    for (int i = 0; i < SEGMENT_COUNT; i++)
    {
        gpio_pin_set_dt(&tens_segments[i], 0);
        gpio_pin_set_dt(&units_segments[i], 0);
        gpio_pin_set_dt(&decimal_segments[i], 0);
    }

    return 0;
}

// 设置数码管显示数字
static void set_digit_display(const struct gpio_dt_spec *segments_array, unsigned char digit)
{
    // 参数检查，防止数组越界
    if (digit > 9)
    {
        LOG_ERR("Digit out of range: %d", digit);
        // 熄灭所有段
        for (int i = 0; i < SEGMENT_COUNT; i++)
        {
            gpio_pin_set_dt(&segments_array[i], 0);
        }
        return;
    }

    unsigned char segments = digit_segments[digit];

    // 设置段选 (使用数组循环设置段码的 4 位)
    for (int i = 0; i < SEGMENT_COUNT; i++)
    {
        gpio_pin_set_dt(&segments_array[i], (segments & (1 << i)) ? 1 : 0);
    }
}

// 显示温度（使用4段数码管，BCD编码）
void display_temp(float temperature)
{
    unsigned int decimal_digit, units_digit, tens_digit, integer_part;
    int temp_int;

    temp_int = (int)(temperature * 10.0f);  // 转换为整数（保留一位小数）

    decimal_digit = temp_int % 10;  // 小数位
    integer_part  = temp_int / 10;  // 整数部分

    // 合理的温度范围检查：0°C 到 64°C (对应 fugaijin.c 中的 pp < 65)
    if (integer_part >= 0 && integer_part < 65)
    {
        units_digit = integer_part % 10;  // 个位
        tens_digit  = integer_part / 10;  // 十位

        // 设置小数位数码管（BCD编码：0-9 对应 0x00-0x09）
        for (int i = 0; i < SEGMENT_COUNT; i++)
        {
            gpio_pin_set_dt(&decimal_segments[i], (decimal_digit >> i) & 0x01);
        }

        // 设置十位和个位数码管（BCD编码）
        // 十位在高4位，个位在低4位
        for (int i = 0; i < SEGMENT_COUNT; i++)
        {
            // 十位（A/B/C/D 对应 bit 0/1/2/3）
            gpio_pin_set_dt(&tens_segments[i], (tens_digit >> i) & 0x01);
            // 个位（A/B/C/D 对应 bit 0/1/2/3）
            gpio_pin_set_dt(&units_segments[i], (units_digit >> i) & 0x01);
        }

        LOG_INF("Temperature: %d.%d", integer_part, decimal_digit);
    }
    else
    {
        // 温度超出范围，熄灭所有数码管
        for (int i = 0; i < SEGMENT_COUNT; i++)
        {
            gpio_pin_set_dt(&tens_segments[i], 0);
            gpio_pin_set_dt(&units_segments[i], 0);
            gpio_pin_set_dt(&decimal_segments[i], 0);
        }
    }
}