/**
 * @file ds18b20.c
 * @brief DS18B20 1-Wire 数字温度传感器驱动实现
 * 
 * 该文件实现了对 DS18B20 温度传感器的驱动，包括初始化、温度读取、
 * 设备诊断等功能。使用 Zephyr RTOS 的 GPIO 接口与传感器通信。
 */

#include "ds18b20.h"
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/cpu.h>

LOG_MODULE_REGISTER(ds18b20, LOG_LEVEL_INF);

//!< DQ GPIO 设备节点
#define DQ_NODE DT_ALIAS(dq)
static const struct gpio_dt_spec dq = GPIO_DT_SPEC_GET(DQ_NODE, gpios);

/**
 * @brief 微秒级延时函数
 * @param us 延时时间（微秒）
 */
static void delay_us(unsigned int us)
{
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us)
    {
    }
}

/**
 * @brief 初始化 TIM2 定时器用于微秒级延时
 */
static void tim2_init_us(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 71;
    TIM2->ARR = 0xFFFF;
    TIM2->CR1 = TIM_CR1_CEN;
}

/**
 * @brief DS18B20 连接诊断函数
 * 
 * 该函数用于诊断 DS18B20 传感器连接状态，检查 GPIO 设备是否就绪，
 * 并确认 DQ 引脚是否被上拉。
 */
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
    k_msleep(1);
    gpio_pin_set_dt(&dq, 1);
    k_msleep(1);

    gpio_pin_configure_dt(&dq, GPIO_INPUT);
    int pin_state = gpio_pin_get_dt(&dq);
    LOG_INF("DQ pin state (should be high with pull-up): %d", pin_state);

    gpio_pin_configure_dt(&dq, GPIO_OUTPUT);
    gpio_pin_set_dt(&dq, 1);

    LOG_INF("=== End Diagnostic ===");
}

/**
 * @brief 初始化 DS18B20 传感器
 * 
 * 该函数执行 DS18B20 的复位和存在检测时序，确保传感器正常连接。
 * 
 * @return 0 表示成功，-1 表示失败
 */
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
                k_msleep(20);  // 增加重试间隔
            }
        }
    }

    LOG_ERR("DS18B20 initialization failed after %d attempts", max_retries);

    return -1;
}

/**
 * @brief 从 DS18B20 读取一个位
 * 
 * 实现 1-Wire 协议中的位读取时序。
 * 
 * @return 读取到的位值 (0 或 1)
 */
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

/**
 * @brief 从 DS18B20 读取一个字节
 * 
 * 通过连续读取 8 个位来组成一个字节。
 * 
 * @return 读取到的字节值
 */
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

/**
 * @brief 向 DS18B20 写入一个字节
 * 
 * 实现 1-Wire 协议中的字节写入时序。
 * 
 * @param dat 要写入的字节值
 */
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

/**
 * @brief 初始化 DS18B20 传感器
 * 
 * 初始化用于与 DS18B20 通信的硬件资源（定时器和 GPIO），
 * 并执行传感器的存在检测。
 * 
 * @return 0 表示成功，-1 表示失败
 */
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

/**
 * @brief 计算 CRC8 校验值
 * 
 * 使用 CRC8 算法计算数据校验值，用于验证 DS18B20 读取的数据完整性。
 * 
 * @param data 要校验的数据指针
 * @param len 数据长度
 * @return CRC8 校验值
 */
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

/**
 * @brief 读取 DS18B20 温度值
 * 
 * 从 DS18B20 传感器读取温度值。该函数执行完整的温度转换和读取时序，
 * 包括复位、发送命令、启动转换、读取暂存器和 CRC 校验。
 * 
 * @return 温度值（摄氏度），如果读取失败则返回 -999.0f
 */
float ds18b20_read_temperature(void)
{
    unsigned int tt;
    float temp_mid;
    unsigned char ramvalue[9];
    int retry_count       = 0;
    const int max_retries = 1;

    // 单次读取温度
    while (retry_count < max_retries)
    {
        if (init_ds18b20() != 0)
        {
            LOG_INF("DS18B20 init failed on read attempt %d", retry_count + 1);
            retry_count++;
            k_msleep(10);
            continue;
        }

        k_msleep(1);
        write_one_char(0xCC);  // 跳过 ROM
        write_one_char(0x44);  // 启动温度转换

        k_msleep(750);  // 等待转换完成

        if (init_ds18b20() != 0)
        {
            LOG_INF("DS18B20 init failed on read attempt %d", retry_count + 1);
            retry_count++;
            k_msleep(10);
            continue;
        }

        k_msleep(1);
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
            k_msleep(10);
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
            k_msleep(10);
            continue;
        }

        return temp_mid;
    }

    // 所有重试失败，返回一个错误值
    LOG_ERR("DS18B20 read failed after %d attempts", max_retries);
    return -999.0f;
}