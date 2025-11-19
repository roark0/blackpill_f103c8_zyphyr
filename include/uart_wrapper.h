#ifndef UART_WRAPPER_H
#define UART_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 发送字符串到 UART
 * @param buf 要发送的字符串
 */
void print_uart(const char *buf);

/**
 * @brief 检查 UART 设备是否就绪
 * @return true 如果设备就绪, false 否则
 */
bool uart_is_ready(void);

/**
 * @brief 发送单个字符到 UART
 * @param c 要发送的字符
 */
void uart_send_char(char c);

void uart_send_hex(const char *buf, int len);

/**
 * @brief 发送字符串到 UART
 * @param str 要发送的字符串
 */
void uart_send_string(const char* str);

#endif /* UART_WRAPPER_H */