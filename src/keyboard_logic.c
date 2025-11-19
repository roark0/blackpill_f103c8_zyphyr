#include "keyboard_logic.h"
#include "gpio_control.h"
#include "zephyr/sys/printk.h"
#include <stdio.h>
#include <sys/_intsup.h>
#include <zephyr/kernel.h>
#include "uart_wrapper.h"

volatile unsigned char Key1904c;

/* 静态变量，用于跟踪按键状态，实现边沿检测 */
static uint8_t prev_key_state[4][4] = {0};  // 上次扫描的按键状态，4行x4列

/* 将简单的行列码转换为实际键盘扫描码的辅助函数 */
uint8_t convert_key_code(uint8_t simple_code)
{
    // 提取行和列
    uint8_t row = (simple_code >> 4) & 0x03;  // 取高4位作为行(0-3)
    uint8_t col = simple_code & 0x03;         // 取低4位作为列(0-3)

    // 根据规律计算实际扫描码
    // 高4位：~(1 << row) & 0x0F (行信息)
    // 低4位：~(1 << col) & 0x0F (列信息)
    uint8_t high_nibble = (~(1 << row)) & 0x0F;
    uint8_t low_nibble  = (~(1 << col)) & 0x0F;

    // 合成最终的8位值
    return (high_nibble << 4) | low_nibble;
}

/* 1904C键盘扫描函数 */
UINT8 Scankey1904C(void)
{
    volatile uint8_t tmp = 0x00;
    uint8_t col, row;

    Key1904c = 0x00;

    // 使用逐行扫描的方式：依次将每一行设为低电平，其他行设为高电平
    for (row = 0; row < 4; row++)
    {
        // 将当前行设为低电平，其他行设为高电平
        uint8_t row_pattern = ~(1 << row) & 0x0F;
        GPIO_SetKeyRows(row_pattern);

        // 短暂延时以稳定信号
        k_msleep(1);

        // 读取列输入状态 (KSNS3-KSNS0)
        tmp = GPIO_ReadKeyCols();

        // 检查每列是否有按键状态变化
        for (col = 0; col < 4; col++)
        {
            // 当前按键状态：0=按下（低电平），1=释放（高电平）
            uint8_t current_state  = (tmp & (1 << col)) ? 1 : 0;  // 1=高电平(释放)，0=低电平(按下)
            uint8_t previous_state = prev_key_state[row][col];

            // 检测下降沿：从释放状态变为按下状态（1 -> 0）
            if (previous_state == 1 && current_state == 0)
            {
                // 检测到按键按下边沿，确认按键按下
                // 进行去抖动验证
                k_msleep(10);

                // 再次读取确认按键状态
                uint8_t tmp2          = GPIO_ReadKeyCols();
                uint8_t confirm_state = (tmp2 & (1 << col)) ? 1 : 0;

                if (confirm_state == 0)  // 确认按键仍然按下
                {
                    // 检测到有效的按键按下事件
                    // 先生成简单的行列码
                    uint8_t simple_code = (row << 4) | col;

                    // 使用转换函数将简单码转换为实际键盘扫描码
                    Key1904c = convert_key_code(simple_code);

                    // 更新按键状态
                    prev_key_state[row][col] = 0;  // 按下状态

                    // 恢复所有行线到高电平
                    GPIO_SetKeyRows(0x0F);
                    return TRUE;
                }
            }

            // 更新状态数组
            prev_key_state[row][col] = current_state;
        }
    }

    // 恢复所有行线到高电平
    GPIO_SetKeyRows(0x0F);

    return FALSE;
}

/* 键盘扫描和传输主函数 */
void Keyboard_Scan_And_Transmit(void)
{
    if (Scankey1904C())
    {
        // 使用单次传输发送4字节序列: F0 FF [Key1904c] FF
        char buffer[4] = {0xF0, 0xFF, Key1904c, 0xFF};
        uart_send_hex(buffer, 4);
    }
}