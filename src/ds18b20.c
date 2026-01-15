#include "ds18b20.h"
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/cpu.h>

LOG_MODULE_REGISTER(ds18b20, LOG_LEVEL_INF);

// DQ GPIO 设备节点
#define DQ_NODE DT_ALIAS(dq)
static const struct gpio_dt_spec dq = GPIO_DT_SPEC_GET(DQ_NODE, gpios);

// 延时函数
static void delay_us(unsigned int us)
{
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us)
    {
    }
}

static void delay_ms(unsigned int ms)
{
    k_msleep(ms);
}

// TIM2 定时器初始化
static void tim2_init_us(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 71;
    TIM2->ARR = 0xFFFF;
    TIM2->CR1 = TIM_CR1_CEN;
}

// DS18B20连接诊断函数
void ds18b20_diagnostic(void)
{
    LOG_INF("=== DS18B20 Diagnostic ===");

    if (!device_is_ready(dq.port))
    {
        LOG_ERR("DQ GPIO device not ready");
        return;
    }
    LOG_INF("DQ GPIO device ready");

    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
    gpio_pin_set_dt(&dq, 0);
    delay_ms(1);
    gpio_pin_set_dt(&dq, 1);
    delay_ms(1);

    gpio_pin_configure_dt(&dq, GPIO_INPUT);
    int pin_state = gpio_pin_get_dt(&dq);
    LOG_INF("DQ pin state (should be high with pull-up): %d", pin_state);

    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
    gpio_pin_set_dt(&dq, 1);

    LOG_INF("=== End Diagnostic ===");
}

// 初始化 DS18B20
static int init_ds18b20(void)
{
    int x                 = 0;
    int retry_count       = 0;
    const int max_retries = 3;

    while (retry_count < max_retries)
    {
        LOG_DBG("DS18B20 initialization attempt %d/%d", retry_count + 1, max_retries);

        gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
        gpio_pin_set_dt(&dq, 1);
        delay_us(10);

        gpio_pin_set_dt(&dq, 0);
        delay_us(480);

        gpio_pin_set_dt(&dq, 1);
        delay_us(70);

        gpio_pin_configure_dt(&dq, GPIO_INPUT);
        x = gpio_pin_get_dt(&dq);

        delay_us(410);

        gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
        gpio_pin_set_dt(&dq, 1);

        if (x == 0)
        {
            return 0;
        }
        else
        {
            LOG_WRN("DS18B20 not detected on attempt %d", retry_count + 1);
            retry_count++;
            if (retry_count < max_retries)
            {
                delay_ms(10);
            }
        }
    }

    LOG_ERR("DS18B20 initialization failed after %d attempts", max_retries);

    return -1;
}

// 读取一个位
static int tmpread_bit(void)
{
    int dat;

    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
    gpio_pin_set_dt(&dq, 0);
    delay_us(2);

    gpio_pin_set_dt(&dq, 1);
    delay_us(2);

    gpio_pin_configure_dt(&dq, GPIO_INPUT);
    delay_us(5);
    dat = gpio_pin_get_dt(&dq);

    delay_us(50);

    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
    gpio_pin_set_dt(&dq, 1);

    return dat;
}

// 读取一个字节
static unsigned char read_one_char(void)
{
    unsigned char i, j, dat;
    dat = 0;

    for (i = 1; i <= 8; i++)
    {
        j   = tmpread_bit();
        dat = (j << 7) | (dat >> 1);
    }

    return dat;
}

// 写一个字节
static void write_one_char(unsigned char dat)
{
    int testb;

    for (int j = 1; j <= 8; j++)
    {
        testb = dat & 0x01;
        dat >>= 1;

        if (testb)
        {
            gpio_pin_set_dt(&dq, 0);
            delay_us(1);
            gpio_pin_set_dt(&dq, 1);
            delay_us(60);
        }
        else
        {
            gpio_pin_set_dt(&dq, 0);
            delay_us(60);
            gpio_pin_set_dt(&dq, 1);
            delay_us(1);
        }
    }
}

// 公共接口：初始化 DS18B20
int ds18b20_init(void)
{
    tim2_init_us();
    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);

    if (!device_is_ready(dq.port))
    {
        LOG_ERR("DQ device not ready");
        return -1;
    }

    return init_ds18b20();
}

// 公共接口：读取温度
float ds18b20_read_temperature(void)
{
    unsigned int tt;
    float temp_mid;
    unsigned char ramvalue[9];

    if (init_ds18b20() != 0)
    {
        LOG_ERR("DS18B20 initialization failed");
        return 0.0f;
    }

    delay_ms(1);
    write_one_char(0xCC);
    write_one_char(0x44);

    delay_ms(750);

    if (init_ds18b20() != 0)
    {
        LOG_ERR("DS18B20 initialization failed");
        return 0.0f;
    }

    delay_ms(1);
    write_one_char(0xCC);
    write_one_char(0xBE);

    for (int i = 0; i < 9; i++)
    {
        ramvalue[i] = read_one_char();
    }

    tt = ramvalue[1];
    tt <<= 8;
    tt += ramvalue[0];

    temp_mid = tt / 2.0f - 0.25f + (float)(ramvalue[7] - ramvalue[6]) / (float)ramvalue[7];

    return temp_mid;
}