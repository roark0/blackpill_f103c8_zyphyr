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
    const int max_retries = 5;

    while (retry_count < max_retries)
    {
        LOG_DBG("DS18B20 initialization attempt %d/%d", retry_count + 1, max_retries);

        // 确保总线空闲
        gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
        gpio_pin_set_dt(&dq, 1);
        delay_us(20);

        // 发送复位脉冲（拉低至少 480μs）
        gpio_pin_set_dt(&dq, 0);
        delay_us(500);

        // 释放总线
        gpio_pin_set_dt(&dq, 1);
        delay_us(15);

        // 等待 DS18B20 响应（15-60μs 后读取）
        delay_us(60);

        // 配置为输入读取存在脉冲
        gpio_pin_configure_dt(&dq, GPIO_INPUT);
        x = gpio_pin_get_dt(&dq);

        // 完成复位时序（等待存在脉冲结束）
        delay_us(420);

        // 重新配置为输出，释放总线
        gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
        gpio_pin_set_dt(&dq, 1);
        delay_us(10);

        // DS18B20 存在时会拉低总线，所以 x 应该为 0
        if (x == 0)
        {
            return 0;
        }
        else
        {
            LOG_INF("DS18B20 not detected on attempt %d", retry_count + 1);
            retry_count++;
            if (retry_count < max_retries)
            {
                delay_ms(20);  // 增加重试间隔
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

    // 主机拉低总线至少 1μs
    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
    gpio_pin_set_dt(&dq, 0);
    delay_us(3);

    // 释放总线，DS18B20 开始发送数据
    gpio_pin_set_dt(&dq, 1);
    delay_us(3);

    // 配置为输入读取数据（在 15μs 内读取）
    gpio_pin_configure_dt(&dq, GPIO_INPUT);
    delay_us(8);  // 等待 DS18B20 稳定输出
    dat = gpio_pin_get_dt(&dq);

    // 完成读取时序（总共至少 60μs）
    delay_us(55);

    // 重新配置为输出，释放总线
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

// CRC 校验函数
static unsigned char crc8(unsigned char *data, unsigned char len)
{
    unsigned char crc = 0;
    unsigned char i;

    while (len--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; i++)
        {
            if (crc & 0x01)
            {
                crc = (crc >> 1) ^ 0x8C;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// 简单的冒泡排序
static void bubble_sort(float *arr, int n)
{
    int i, j;
    float temp;
    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// 公共接口：读取温度
float ds18b20_read_temperature(void)
{
    unsigned int tt;
    float temp_mid;
    unsigned char ramvalue[9];
    int retry_count = 0;
    const int max_retries = 3;
    static float last_valid_temp = 25.0f;  // 保存上一次有效温度
    static bool first_read = true;  // 首次读取标志

    float temp_readings[10];  // 存储10次温度读取值
    int valid_readings = 0;   // 有效读取次数

    // 读取10次温度
    while (valid_readings < 10 && retry_count < max_retries)
    {
        if (init_ds18b20() != 0)
        {
            LOG_INF("DS18B20 init failed on read attempt %d", retry_count + 1);
            retry_count++;
            delay_ms(10);
            continue;
        }

        delay_ms(1);
        write_one_char(0xCC);  // 跳过 ROM
        write_one_char(0x44);  // 启动温度转换

        delay_ms(750);  // 等待转换完成

        if (init_ds18b20() != 0)
        {
            LOG_INF("DS18B20 init failed on read attempt %d", retry_count + 1);
            retry_count++;
            delay_ms(10);
            continue;
        }

        delay_ms(1);
        write_one_char(0xCC);  // 跳过 ROM
        write_one_char(0xBE);  // 读取暂存器

        for (int i = 0; i < 9; i++)
        {
            ramvalue[i] = read_one_char();
        }

        // CRC 校验
        if (crc8(ramvalue, 8) != ramvalue[8])
        {
            LOG_INF("DS18B20 CRC error on read attempt %d", retry_count + 1);
            retry_count++;
            delay_ms(10);
            continue;
        }

        // CRC 校验通过，计算温度
        tt = ramvalue[1];
        tt <<= 8;
        tt += ramvalue[0];

        temp_mid = tt / 2.0f - 0.25f + (float)(ramvalue[7] - ramvalue[6]) / (float)ramvalue[7];

        // 温度范围检查（-55°C 到 125°C）
        if (temp_mid < -55.0f || temp_mid > 125.0f)
        {
            LOG_WRN("DS18B20 temperature out of range: %.2f", (double)temp_mid);
            retry_count++;
            delay_ms(10);
            continue;
        }

        // 温度变化合理性检查（单次变化不应超过 2°C）
        // 首次读取时跳过检查，避免初始值偏差导致的误报
        if (!first_read)
        {
            float temp_change = temp_mid - last_valid_temp;
            if (temp_change > 2.0f || temp_change < -2.0f)
            {
                LOG_WRN("DS18B20 temperature jump detected: %.2f -> %.2f (change: %.2f)",
                        (double)last_valid_temp, (double)temp_mid, (double)temp_change);
                retry_count++;
                delay_ms(10);
                continue;
            }
        }

        // 保存有效温度值
        temp_readings[valid_readings] = temp_mid;
        valid_readings++;
    }

    // 检查是否读取到足够的有效值
    if (valid_readings < 4)
    {
        LOG_ERR("DS18B20 not enough valid readings: %d, using last valid temp", valid_readings);
        return last_valid_temp;
    }

    // 更新首次读取标志
    if (first_read)
    {
        first_read = false;
    }

    // 如果读取次数不足10次，只取实际读取的数量
    int readings_to_sort = valid_readings;
    int readings_to_keep = readings_to_sort - 4;  // 去掉2个最大和2个最小

    // 如果有效读取数不足6个，无法去掉4个值，返回全部的平均值
    if (readings_to_keep < 2)
    {
        readings_to_keep = readings_to_sort;
    }

    // 排序温度值
    bubble_sort(temp_readings, readings_to_sort);

    // 计算中间值的平均值（去掉最大2个和最小2个）
    float sum = 0.0f;
    int start_index = 2;
    int end_index = readings_to_sort - 2;

    // 确保索引有效
    if (end_index > start_index)
    {
        for (int i = start_index; i < end_index; i++)
        {
            sum += temp_readings[i];
        }

        last_valid_temp = sum / (end_index - start_index);
        LOG_DBG("DS18B20 final average: %.2f", (double)last_valid_temp);
    }
    else
    {
        last_valid_temp = sum / readings_to_sort;
        LOG_DBG("DS18B20 final average: %.2f", (double)last_valid_temp);
    }

    return last_valid_temp;
}