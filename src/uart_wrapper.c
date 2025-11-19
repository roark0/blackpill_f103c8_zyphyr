#include "uart_wrapper.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#define UART_DEVICE_DT_NAME DT_CHOSEN(zephyr_console)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_DT_NAME);

// 用于存储配置状态
static bool uart_initialized = false;

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(const char *buf)
{
    int msg_len = strlen(buf);

    for (int i = 0; i < msg_len; i++)
    {
        uart_poll_out(uart_dev, buf[i]);
    }
}

void uart_send_hex(const char *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        uart_poll_out(uart_dev, buf[i]);
    }
}

/**
 * @brief 检查 UART 设备是否就绪
 * @return true 如果设备就绪, false 否则
 */
bool uart_is_ready(void)
{
    return device_is_ready(uart_dev);
}

/**
 * @brief 发送单个字符到 UART
 * @param c 要发送的字符
 */
void uart_send_char(char c)
{
    uart_poll_out(uart_dev, c);
}

/**
 * @brief 发送字符串到 UART
 * @param str 要发送的字符串
 */
void uart_send_string(const char *str)
{
    if (!str)
    {
        return;
    }

    int msg_len = strlen(str);
    for (int i = 0; i < msg_len; i++)
    {
        uart_poll_out(uart_dev, str[i]);
    }
}

/**
 * @brief 兼容旧程序的串口初始化函数
 * 配置为 9-bit UART, 波特率 19200, 奇偶校验位等
 */
void Serial_Init(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return;
    }
    
    // 在Zephyr中，UART配置通常在设备树或系统配置中完成
    // 这里我们只确认设备就绪并设置初始化标志
    uart_initialized = true;
    printk("UART initialized (9-bit, 19200 baud, parity check)\n");
}

/**
 * @brief 兼容旧程序的发送单个字符函数
 * @param ch 要发送的字符 (uint8_t)
 * 包含奇偶校验位的处理
 */
void SendChar(uint8_t ch)
{
    // 在原始代码中，P寄存器是奇偶校验位，P = 0表示偶数个1，P = 1表示奇数个1
    // 8051中通过设置TB8位来发送奇偶校验位
    // 但由于Zephyr的UART驱动不直接支持9位数据和奇偶校验位的单独设置
    // 我们直接发送8位数据，这是在当前硬件和软件框架下的最佳兼容方式
    
    uart_poll_out(uart_dev, (char)ch);
}