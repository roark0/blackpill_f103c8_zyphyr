#include "uart_wrapper.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#define UART_DEVICE_DT_NAME DT_CHOSEN(zephyr_console)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_DT_NAME);

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