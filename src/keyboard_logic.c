#include "keyboard_logic.h"
#include "gpio_control.h"
#include "zephyr/sys/printk.h"
#include <stdio.h>
#include <sys/_intsup.h>
#include <zephyr/kernel.h>
#include "uart_wrapper.h"

// 吸液键 4E
// KSCON0 PB4 output 
// KSNS2 PB2 input

// 走纸键 5D
// KSCON3 PB7 output
// KSNS1 PB1 input

// 冲洗键 55
// KSCON1 PB5 output
// KSNS1 PB1 input

// 检测原理：以上gpio默认为高电平，依次拉低output,检测对应的input,检测到低电平则按键被按下

// 按键状态数组，用于检测按键状态变化 (6行x8列)
static uint8_t prev_key_state[6][8] = {0};  // 6行对应PB8-PB13, 8列对应PA0-PA7

// 特殊按键的前一个状态，用于边沿检测
static uint8_t prev_special_key_state[3] = {0, 0, 0};  // 吸液键、走纸键、冲洗键的前一状态

/* 全局变量声明 */
volatile unsigned int gKey_Buffer;   //common keycode buffer
volatile unsigned int gFunc_Buffer;  //function keycode buffer
volatile uint8_t Flag_Updown;        //the flag of function key keeping down or not
volatile uint8_t Flag_Key;           //the flag of any key keeping down or not

volatile unsigned char Key9000c;
volatile unsigned char Key1904c;
volatile char G_uc9000ScanInterval;

//定义TRUE和FALSE
#define TRUE 1
#define FALSE 0

/* 键值转换函数：将行列位置转换为旧版键值 */
unsigned int convert_key_to_legacy_format(uint8_t row, uint8_t col)
{
    // 在旧版keyboard.c中，键值是通过行列扫描状态生成的
    // 首先需要模拟旧版的扫描过程来生成对应的键值

    // 根据键盘布局分析，我们知道每个按键的行列位置
    // 现在我们需要从行列位置得到旧版的键值

    // 每个按键位置对应的旧版键值
    switch ((row << 4) | col)  // 将行列组合成一个值用于switch
    {
        case 0x00:
            return 0xFEFE;  // Shift (0,0)
        case 0x01:
            return 0xFDFE;  // Z (0,1)
        case 0x02:
            return 0xFBFE;  // X (0,2)
        case 0x03:
            return 0xF7FE;  // C (0,3)
        case 0x04:
            return 0xEFFE;  // V (0,4)
        case 0x05:
            return 0xDFFE;  // B (0,5)
        case 0x06:
            return 0xBFFE;  // N (0,6)
        case 0x07:
            return 0x7FFE;  // M (0,7)

        case 0x10:
            return 0xFEFD;  // Capslock (1,0)
        case 0x11:
            return 0xFDFD;  // A (1,1)
        case 0x12:
            return 0xFBFD;  // S (1,2)
        case 0x13:
            return 0xF7FD;  // D (1,3)
        case 0x14:
            return 0xEFFD;  // F (1,4)
        case 0x15:
            return 0xDFFD;  // G (1,5)
        case 0x16:
            return 0xBFFD;  // H (1,6)
        case 0x17:
            return 0x7FFD;  // J (1,7)

        case 0x20:
            return 0xFEFB;  // TAB (2,0)
        case 0x21:
            return 0xFDFB;  // Q (2,1)
        case 0x22:
            return 0xFBFB;  // W (2,2)
        case 0x23:
            return 0xF7FB;  // E (2,3)
        case 0x24:
            return 0xEFFB;  // R (2,4)
        case 0x25:
            return 0xDFFB;  // T (2,5)
        case 0x26:
            return 0xBFFB;  // Y (2,6)
        case 0x27:
            return 0x7FFB;  // U (2,7)

        case 0x30:
            return 0xFEF7;  // 1 (3,0)
        case 0x31:
            return 0xFDF7;  // 2 (3,1)
        case 0x32:
            return 0xFBF7;  // 3 (3,2)
        case 0x33:
            return 0xF7F7;  // 4 (3,3)
        case 0x34:
            return 0xEFF7;  // 5 (3,4)
        case 0x35:
            return 0xDFF7;  // 6 (3,5)
        case 0x36:
            return 0xBFF7;  // 7 (3,6)
        case 0x37:
            return 0x7FF7;  // 8 (3,7)

        case 0x40:
            return 0xFEEF;  // LCtrl (4,0)
        case 0x41:
            return 0xFDEF;  // 0 (4,1)
        case 0x42:
            return 0xFBEF;  // K (4,2)
        case 0x43:
            return 0xF7EF;  // Dot (4,3)
        case 0x44:
            return 0xEFEF;  // Space (4,4)
        case 0x45:
            return 0xDFEF;  // Enter (4,5)
        case 0x46:
            return 0xBFEF;  // I (4,6)
        case 0x47:
            return 0x7FEF;  // 9 (4,7)

        case 0x50:          /* 无按键 */
            return 0xFFFF;  // (5,0)
        case 0x51:          /* 无按键 */
            return 0xFFFF;  // (5,1)
        case 0x52:
            return 0xFBDF;  // P (5,2)
        case 0x53:
            return 0xF7DF;  // L (5,3)
        case 0x54:          /* 无按键 */
            return 0xFFFF;  // (5,4)
        case 0x55:          /* 无按键 */
            return 0xFFFF;  // (5,5)
        case 0x56:
            return 0xBFDF;  // O (5,6)
        case 0x57:
            return 0x7FDF;  // Backspace (5,7)

        default:
            return 0xFFFF;  // 无效键值
    }
}

/* 9000键盘扫描函数 */
UINT8 Key_Scan(void)
{
    volatile uint8_t tmp = 0x00;
    uint8_t col, row;
    uint8_t key_pressed     = FALSE;  // 标记是否检测到按键按下事件
    uint8_t any_key_pressed = FALSE;  // 标记是否有按键处于按下状态

    // 使用逐行扫描的方式：依次将每一行设为低电平，其他行设为高电平
    for (row = 0; row < 6; row++)
    {
        // 将当前行设为低电平，其他行设为高电平
        uint8_t row_pattern = ~(1 << row) & 0x3F;  // 0x3F = 0b00111111，对应6个行 (PB8-PB13)
        GPIO_SetKeyRows(row_pattern);

        // 短暂延时以稳定信号
        k_msleep(1);

        // 读取列输入状态 (PA0-PA7)
        tmp = GPIO_ReadKeyCols();

        // 检查每列是否有按键状态变化
        for (col = 0; col < 8; col++)
        {
            // 当前按键状态：0=释放（高电平），1=按下（低电平）
            uint8_t current_state  = (tmp & (1 << col)) ? 1 : 0;  // 0=高电平(释放)，1=低电平(按下)
            uint8_t previous_state = prev_key_state[row][col];

            // 检测下降沿：从释放状态变为按下状态（0 -> 1）
            if (previous_state == 0 && current_state == 1)
            {
                // 检测到按键按下边沿，确认按键按下
                // 进行去抖动验证
                k_msleep(10);

                // 再次读取确认按键状态
                uint8_t tmp2          = GPIO_ReadKeyCols();
                uint8_t confirm_state = (tmp2 & (1 << col)) ? 1 : 0;

                if (confirm_state == 1)  // 确认按键仍然按下
                {
                    // 检测到有效的按键按下事件
                    // 将行列信息编码为扫描码，并转换为旧版键值
                    unsigned int legacy_key = convert_key_to_legacy_format(row, col);

                    if (legacy_key != 0xFFFF)  // 确保是有效的按键
                    {
                        gKey_Buffer = legacy_key;

                        Flag_Key = TRUE;  // 标记有按键按下

                        key_pressed = TRUE;  // 标记已检测到按键按下事件

                        printk("Key_Scan: Key pressed at Row %d, Col %d, gKey_Buffer=0x%04X\n", row, col, gKey_Buffer);
                    }
                }
            }

            // 检查当前按键是否处于按下状态（用于更新全局标志）
            if (current_state == 1)  // 如果当前为按下状态（低电平）
            {
                any_key_pressed = TRUE;
            }

            // 更新状态数组 - 放在按键按下检测和全局状态检查之后
            prev_key_state[row][col] = current_state;
        }
    }

    // 如果没有按键被按下，更新全局标志
    if (!any_key_pressed)
    {
        Flag_Key = FALSE;
    }

    // 恢复所有行线到高电平
    GPIO_SetKeyRows(0x3F);

    // 只有检测到新的按键按下事件时才返回TRUE
    return key_pressed;
}

/* 按键解码函数 */
unsigned int Key_Decode(unsigned int key)
{
    printk("Key_Decode: received key=0x%04X\n", key);
    volatile unsigned int iKeyID = 0;

    // 扫描码的断码加载在高字节,通码加载在低字节
    switch (key)
    {

        case 0xfdef:
            iKeyID = 0xF045;
            printk("0\n");
            break;  //0

        case 0xfef7:
            iKeyID = 0xF016;
            printk("1\n");
            break;  //1
        case 0xfdf7:
            iKeyID = 0xF01E;
            printk("2\n");
            break;  //2
        case 0xfbf7:
            iKeyID = 0xF026;
            printk("3\n");
            break;  //3
        case 0xf7f7:
            iKeyID = 0xF025;
            printk("4\n");
            break;  //4
        case 0xeff7:
            iKeyID = 0xF02E;
            printk("5\n");
            break;  //5
        case 0xdff7:
            iKeyID = 0xF036;
            printk("6\n");
            break;  //6
        case 0xbff7:
            iKeyID = 0xF03D;
            printk("7\n");
            break;  //7
        case 0x7ff7:
            iKeyID = 0xF03E;
            printk("8\n");
            break;  //8
        case 0x7fef:
            iKeyID = 0xF046;
            printk("9\n");
            break;  //9
        case 0xfdfd:
            iKeyID = 0xF01C;
            printk("A\n");
            break;  //A
        case 0xdffe:
            iKeyID = 0xF032;
            printk("B\n");
            break;  //B
        case 0xf7fe:
            iKeyID = 0xF021;
            printk("C\n");
            break;  //C
        case 0xf7fd:
            iKeyID = 0xF023;
            printk("D\n");
            break;  //D
        case 0xf7fb:
            iKeyID = 0xF024;
            printk("E\n");
            break;  //E
        case 0xeffd:
            iKeyID = 0xF02B;
            printk("F\n");
            break;  //F
        case 0xdffd:
            iKeyID = 0xF034;
            printk("G\n");
            break;  //G
        case 0xbffd:
            iKeyID = 0xF033;
            printk("H\n");
            break;  //H
        case 0xbfef:
            iKeyID = 0xF043;
            printk("I\n");
            break;  //I
        case 0x7ffd:
            iKeyID = 0xF03B;
            printk("J\n");
            break;  //J
        case 0xfbef:
            iKeyID = 0xF042;
            printk("K\n");
            break;  //K
        case 0xf7df:
            iKeyID = 0xF04B;
            printk("L\n");
            break;  //L
        case 0x7ffe:
            iKeyID = 0xF03A;
            printk("M\n");
            break;  //M
        case 0xbffe:
            iKeyID = 0xF031;
            printk("N\n");
            break;  //N
        case 0xbfdf:
            iKeyID = 0xF044;
            printk("O\n");
            break;  //O
        case 0xfbdf:
            iKeyID = 0xF04D;
            printk("P\n");
            break;  //P
        case 0xfdfb:
            iKeyID = 0xF015;
            printk("Q\n");
            break;  //Q
        case 0xeffb:
            iKeyID = 0xF02D;
            printk("R\n");
            break;  //R
        case 0xfbfd:
            iKeyID = 0xF01B;
            printk("S\n");
            break;  //S
        case 0xdffb:
            iKeyID = 0xF02C;
            printk("T\n");
            break;  //T
        case 0x7ffb:
            iKeyID = 0xF03C;
            printk("U\n");
            break;  //U
        case 0xeffe:
            iKeyID = 0xF02A;
            printk("V\n");
            break;  //V
        case 0xfbfb:
            iKeyID = 0xF01D;
            printk("W\n");
            break;  //W
        case 0xfbfe:
            iKeyID = 0xF022;
            printk("X\n");
            break;  //X
        case 0xbffb:
            iKeyID = 0xF035;
            printk("Y\n");
            break;  //Y
        case 0xfdfe:
            iKeyID = 0xF01A;
            printk("Z\n");
            break;  //Z
        case 0xdfef:
            iKeyID = 0xF05A;
            printk("enter\n");
            break;  //enter
        case 0xefef:
            iKeyID = 0xF029;
            printk("space\n");
            break;  //space
        case 0xf7ef:
            iKeyID = 0xF049;
            printk("dot\n");
            break;  //dot
        case 0x7fdf:
            iKeyID = 0xF066;
            printk("backspace\n");
            break;  //backspace
        case 0xfefb:
            iKeyID = 0xF00D;
            printk("TAB\n");
            break;  //TAB
        case 0xfefd:
            iKeyID = 0xF058;
            printk("capslock\n");
            break;  //capslock LED
        default:
            iKeyID = 0;
            break;
    }

    k_msleep(100);

    return iKeyID;
}

/* 功能键解码函数 */
void Function_Key_Decode(unsigned int Func_Key)
{
    switch (Func_Key)
    {
        case 0xFEFE:  //shift
            printk("shift\n");
            while (1)
            {
                Key_Scan();
                if (Flag_Key == FALSE)
                {  //if no key is down the shift key has released and break
                    break;
                }

                if (gKey_Buffer != 0xFEFE)
                {
                    if ((gKey_Buffer & 0xFF) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0x01FE;  //the common key is the same line as the shift
                    }
                    else if (((gKey_Buffer >> 8) & 0xFE) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0xFE01;  //the common key is the same column as the shift
                    }
                    else
                    {
                        gKey_Buffer = gKey_Buffer | 0x0101;  //the common key isn't the same
                    }

                    gFunc_Buffer = 0xF012;  // the bread and make code of shift key

                    Flag_Updown = 1;  //the symbol of the function key down
                    break;
                }
            }
            break;  //LShift is down
        case 0xFEEF:
            printk("Ctrl\n");
            while (1)
            {
                Key_Scan();
                if (Flag_Key == FALSE)
                {
                    break;
                }

                if (gKey_Buffer != 0xFEEF)
                {
                    if ((gKey_Buffer & 0xFF) == 0xEF)
                    {
                        gKey_Buffer = gKey_Buffer | 0x01EF;
                    }
                    else if (((gKey_Buffer >> 8) & 0xff) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0xFE10;
                    }
                    else
                    {
                        gKey_Buffer = gKey_Buffer | 0x0110;
                    }

                    gFunc_Buffer = 0xF014;

                    Flag_Updown = 1;
                    break;
                }
            }
            break;  //LCtrl
        default:
            Flag_Updown = 0;
            break;  //no function key is down
    }
}

/* 检测特殊按键函数 */
UINT8 ScanSpecialKeys(void)
{
    volatile unsigned char tmp = 0x00;
    uint8_t special_key_pressed = FALSE;
    
    // 特殊按键的扫描码映射
    // 0=吸液键 (0x4E), 1=走纸键 (0x5D), 2=冲洗键 (0x55)
    const uint8_t key_codes[3] = {0x4E, 0x5D, 0x55};
    
    // 检测每个特殊按键
    for (int i = 0; i < 3; i++)
    {
        // 如果已有按键被按下，跳过后续按键检测
        if (special_key_pressed)
        {
            break;
        }
        
        // 拉低当前按键的输出引脚
        GPIO_SetSpecialKeyOutput(i, 0); // 0=低电平
        k_msleep(1); // 短暂延时稳定信号
        
        // 根据按键类型读取对应的输入引脚
        // 注意：走纸键和冲洗键共用同一个输入引脚 (KSNS1)
        uint8_t input_idx = (i == 0) ? 0 : 1; // 吸液键使用输入0，走纸/冲洗键使用输入1
        tmp = GPIO_ReadSpecialKeyInput(input_idx);
        
        // 检测下降沿：从释放状态变为按下状态（0 -> 1）
        uint8_t current_state = tmp;
        uint8_t previous_state = prev_special_key_state[i];
        
        // 只有在检测到下降沿时才处理按键按下
        if (previous_state == 0 && current_state == 1)
        {
            // 确认按键按下（防抖）
            k_msleep(10);
            tmp = GPIO_ReadSpecialKeyInput(input_idx);
            if (tmp == 1)  // 确认按键仍然按下
            {
                Key9000c = key_codes[i];  // 设置按键码
                special_key_pressed = TRUE;
                
                // 打印按键信息
                if (i == 0) {
                    printk("Suction key pressed (0x%02X)\n", Key9000c);
                } else if (i == 1) {
                    printk("Paper feed key pressed (0x%02X)\n", Key9000c);
                } else if (i == 2) {
                    printk("Flush key pressed (0x%02X)\n", Key9000c);
                }
            }
        }
        
        // 更新按键状态
        prev_special_key_state[i] = current_state;
        
        // 恢复当前按键输出引脚到高电平
        GPIO_SetSpecialKeyOutput(i, 1);
    }
    
    return special_key_pressed;
}

/* 9000其他按键扫描函数 */
UINT8 ScanOthekey9000(void)
{
    // 使用新的特殊按键检测函数
    return ScanSpecialKeys();
}

/* 键盘扫描和传输主函数 */
void Keyboard_Scan_And_Transmit(void)
{
    volatile unsigned int Key;

    if (Key_Scan())
    {
        Function_Key_Decode(gKey_Buffer);  //judge whether the function key is down or not
        Key = Key_Decode(gKey_Buffer);     //decode the common keycode
        if (Flag_Key == TRUE)
        {  //if a function key is down the makecode is transmitted first then the common or second key makecode is transmitted
            //at last the second breakcode is transmitted followed by the first function key's breakcode
            if (Flag_Updown == 1)
            {
                printk("Function key detected, sending function key codes\n");
                uart_send_char((gFunc_Buffer >> 8) & 0xff);

                uart_send_char(gFunc_Buffer & 0xff);
                Flag_Updown = 0;  //must clear the flag of the function keeping down
            }
            else
            {
                printk("Regular key, sending key codes: 0x%02X%02X\n", (Key >> 8) & 0xff, Key & 0xff);
                uart_send_char((Key >> 8) & 0xff);
                uart_send_char(Key & 0xff);
            }

            uart_send_char((Key >> 8) & 0xff);
            uart_send_char(Key & 0xff);
        }
    }
    else if (ScanOthekey9000())
    {
        printk("Special key detected, sending codes: 0xF0 0x%02X\n", Key9000c);
        uart_send_char(0xF0);
        uart_send_char(Key9000c);

        uart_send_char(0xF0);
        uart_send_char(Key9000c);
    }
}