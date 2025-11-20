#include "keyboard_logic.h"
#include "gpio_control.h"
#include "zephyr/sys/printk.h"
#include <stdio.h>
#include <sys/_intsup.h>
#include <zephyr/kernel.h>
#include "uart_wrapper.h"

// 按键状态数组，用于检测按键状态变化 (6行x8列)
static uint8_t prev_key_state[6][8] = {0};  // 6行对应PB8-PB13, 8列对应PA0-PA7

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

/* 9000键盘扫描函数 */
UINT8 Key_Scan(void)
{
    volatile uint8_t tmp = 0x00;
    uint8_t col, row;
    uint8_t key_pressed = FALSE;  // 标记是否检测到按键按下事件
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
            uint8_t current_state = (tmp & (1 << col)) ? 1 : 0;  // 0=高电平(释放)，1=低电平(按下)
            uint8_t previous_state = prev_key_state[row][col];

            // 检测下降沿：从释放状态变为按下状态（0 -> 1）
            if (previous_state == 0 && current_state == 1)
            {
                // 检测到按键按下边沿，确认按键按下
                // 进行去抖动验证
                k_msleep(10);

                // 再次读取确认按键状态
                uint8_t tmp2 = GPIO_ReadKeyCols();
                uint8_t confirm_state = (tmp2 & (1 << col)) ? 1 : 0;

                if (confirm_state == 1)  // 确认按键仍然按下
                {
                    // 检测到有效的按键按下事件
                    // 将行列信息编码为扫描码
                    gKey_Buffer = ((uint16_t)(1 << col) << 8) | (1 << row);  // 高8位为列信息，低8位为行信息
                    
                    Flag_Key = TRUE;  // 标记有按键按下

                    key_pressed = TRUE;  // 标记已检测到按键按下事件

                    printk("Key_Scan: Key pressed at Row %d, Col %d, gKey_Buffer=0x%04X\n", row, col, gKey_Buffer);
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
    return key_pressed ? TRUE : FALSE;
}

/* 按键解码函数 */
unsigned int Key_Decode(unsigned int key)
{
    volatile unsigned int iKeyID = 0;

    printk("Input key = 0x%04X\n", key);
    // 扫描码的断码加载在高字节,通码加载在低字节
    switch (key)
    {
        case 0xfdef:
            iKeyID = 0xF045;
            printk("Matched key 0 -> 0xF045\n");
            break;  //0
        case 0xfef7:
            iKeyID = 0xF016;
            printk("Matched key 1 -> 0xF016\n");
            break;  //1
        case 0xfdf7:
            iKeyID = 0xF01E;
            printk("Matched key 2 -> 0xF01E\n");
            break;  //2
        case 0xfbf7:
            iKeyID = 0xF026;
            printk("Matched key 3 -> 0xF026\n");
            break;  //3
        case 0xf7f7:
            iKeyID = 0xF025;
            printk("Matched key 4 -> 0xF025\n");
            break;  //4
        case 0xeff7:
            iKeyID = 0xF02E;
            printk("Matched key 5 -> 0xF02E\n");
            break;  //5
        case 0xdff7:
            iKeyID = 0xF036;
            printk("Matched key 6 -> 0xF036\n");
            break;  //6
        case 0xbff7:
            iKeyID = 0xF03D;
            printk("Matched key 7 -> 0xF03D\n");
            break;  //7
        case 0x7ff7:
            iKeyID = 0xF03E;
            printk("Matched key 8 -> 0xF03E\n");
            break;  //8
        case 0x7fef:
            iKeyID = 0xF046;
            printk("Matched key 9 -> 0xF046\n");
            break;  //9
        case 0xfdfd:
            iKeyID = 0xF01C;
            printk("Matched key A -> 0xF01C\n");
            break;  //A
        case 0xdffe:
            iKeyID = 0xF032;
            printk("Matched key B -> 0xF032\n");
            break;  //B
        case 0xf7fe:
            iKeyID = 0xF021;
            printk("Matched key C -> 0xF021\n");
            break;  //C
        case 0xf7fd:
            iKeyID = 0xF023;
            printk("Matched key D -> 0xF023\n");
            break;  //D
        case 0xf7fb:
            iKeyID = 0xF024;
            printk("Matched key E -> 0xF024\n");
            break;  //E
        case 0xeffd:
            iKeyID = 0xF02B;
            printk("Matched key F -> 0xF02B\n");
            break;  //F
        case 0xdffd:
            iKeyID = 0xF034;
            printk("Matched key G -> 0xF034\n");
            break;  //G
        case 0xbffd:
            iKeyID = 0xF033;
            printk("Matched key H -> 0xF033\n");
            break;  //H
        case 0xbfef:
            iKeyID = 0xF043;
            printk("Matched key I -> 0xF043\n");
            break;  //I
        case 0x7ffd:
            iKeyID = 0xF03B;
            printk("Matched key J -> 0xF03B\n");
            break;  //J
        case 0xfbef:
            iKeyID = 0xF042;
            printk("Matched key K -> 0xF042\n");
            break;  //K
        case 0xf7df:
            iKeyID = 0xF04B;
            printk("Matched key L -> 0xF04B\n");
            break;  //L
        case 0x7ffe:
            iKeyID = 0xF03A;
            printk("Matched key M -> 0xF03A\n");
            break;  //M
        case 0xbffe:
            iKeyID = 0xF031;
            printk("Matched key N -> 0xF031\n");
            break;  //N
        case 0xbfdf:
            iKeyID = 0xF044;
            printk("Matched key O -> 0xF044\n");
            break;  //O
        case 0xfbdf:
            iKeyID = 0xF04D;
            printk("Matched key P -> 0xF04D\n");
            break;  //P
        case 0xfdfb:
            iKeyID = 0xF015;
            printk("Matched key Q -> 0xF015\n");
            break;  //Q
        case 0xeffb:
            iKeyID = 0xF02D;
            printk("Matched key R -> 0xF02D\n");
            break;  //R
        case 0xfbfd:
            iKeyID = 0xF01B;
            printk("Matched key S -> 0xF01B\n");
            break;  //S
        case 0xdffb:
            iKeyID = 0xF02C;
            printk("Matched key T -> 0xF02C\n");
            break;  //T
        case 0x7ffb:
            iKeyID = 0xF03C;
            printk("Matched key U -> 0xF03C\n");
            break;  //U
        case 0xeffe:
            iKeyID = 0xF02A;
            printk("Matched key V -> 0xF02A\n");
            break;  //V
        case 0xfbfb:
            iKeyID = 0xF01D;
            printk("Matched key W -> 0xF01D\n");
            break;  //W
        case 0xfbfe:
            iKeyID = 0xF022;
            printk("Matched key X -> 0xF022\n");
            break;  //X
        case 0xbffb:
            iKeyID = 0xF035;
            printk("Matched key Y -> 0xF035\n");
            break;  //Y
        case 0xfdfe:
            iKeyID = 0xF01A;
            printk("Matched key Z -> 0xF01A\n");
            break;  //Z

        case 0xdfef:
            iKeyID = 0xF05A;
            printk("Matched key Enter -> 0xF05A\n");
            break;  //enter
        case 0xefef:
            iKeyID = 0xF029;
            printk("Matched key Space -> 0xF029\n");
            break;  //space
        case 0xf7ef:
            iKeyID = 0xF049;
            printk("Matched key Dot -> 0xF049\n");
            break;  //dot         // 7
        case 0x7fdf:
            iKeyID = 0xF066;
            printk("Matched key Backspace -> 0xF066\n");
            break;  //backspace
        case 0xfefb:
            iKeyID = 0xF00D;
            printk("Matched key TAB -> 0xF00D\n");
            break;  //TAB
        case 0xfefd:
            iKeyID = 0xF058;
            printk("Matched key Capslock LED -> 0xF058\n");
            break;  //capslock LED
        default:
            iKeyID = 0;
            printk("No match found for key 0x%04X\n", key);
            break;
    }

    k_msleep(100);

    printk("Output iKeyID = 0x%04X\n", iKeyID);
    return iKeyID;
}

/* 功能键解码函数 */
void Function_Key_Decode(unsigned int Func_Key)  //Func_Key = gKey_Buffer
{
    printk("Function_Key_Decode: Input Func_Key = 0x%04X\n", Func_Key);

    switch (Func_Key)
    {
        case 0xFEFE:  //shift
            printk("Function_Key_Decode: Shift key detected\n");
            while (1)
            {
                Key_Scan();
                if (Flag_Key == FALSE)
                {  //if no key is down the shift key has released and break
                    printk("Function_Key_Decode: Shift key released\n");
                    break;
                }

                if (gKey_Buffer != 0xFEFE)
                {
                    printk("Function_Key_Decode: Another key pressed with shift\n");
                    if ((gKey_Buffer & 0xFF) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0x01FE;  //the common key is the same line as the shift
                        printk("Function_Key_Decode: Same line key, gKey_Buffer = 0x%04X\n", gKey_Buffer);
                    }
                    else if (((gKey_Buffer >> 8) & 0xFE) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0xFE01;  //the common key is the same column as the shift
                        printk("Function_Key_Decode: Same column key, gKey_Buffer = 0x%04X\n", gKey_Buffer);
                    }
                    else
                    {
                        gKey_Buffer = gKey_Buffer | 0x0101;  //the common key isn't the same
                        printk("Function_Key_Decode: Different line/column key, gKey_Buffer = 0x%04X\n", gKey_Buffer);
                    }

                    gFunc_Buffer = 0xF012;  // the bread and make code of shift key
                    printk("Function_Key_Decode: Set gFunc_Buffer for shift (set 2) = 0x%04X\n", gFunc_Buffer);

                    Flag_Updown = 1;  //the symbol of the function key down
                    printk("Function_Key_Decode: Function key down flag set\n");
                    break;
                }
            }
            break;  //LShift is down
        case 0xFEEF:
            printk("Function_Key_Decode: Ctrl key detected\n");
            while (1)
            {
                Key_Scan();
                if (Flag_Key == FALSE)
                {
                    printk("Function_Key_Decode: Ctrl key released\n");
                    break;
                }

                if (gKey_Buffer != 0xFEEF)
                {
                    printk("Function_Key_Decode: Another key pressed with ctrl\n");
                    if ((gKey_Buffer & 0xFF) == 0xEF)
                    {
                        gKey_Buffer = gKey_Buffer | 0x01EF;
                        printk("Function_Key_Decode: Same line key, gKey_Buffer = 0x%04X\n", gKey_Buffer);
                    }
                    else if (((gKey_Buffer >> 8) & 0xff) == 0xFE)
                    {
                        gKey_Buffer = gKey_Buffer | 0xFE10;
                        printk("Function_Key_Decode: Same column key, gKey_Buffer = 0x%04X\n", gKey_Buffer);
                    }
                    else
                    {
                        gKey_Buffer = gKey_Buffer | 0x0110;
                        printk("Function_Key_Decode: Different line/column key, gKey_Buffer = 0x%04X\n", gKey_Buffer);
                    }

                    gFunc_Buffer = 0xF014;
                    printk("Function_Key_Decode: Set gFunc_Buffer for ctrl (set 2) = 0x%04X\n", gFunc_Buffer);

                    Flag_Updown = 1;
                    printk("Function_Key_Decode: Function key down flag set\n");
                    break;
                }
            }
            break;  //LCtrl
        default:
            printk("Function_Key_Decode: No function key detected\n");
            Flag_Updown = 0;
            break;  //no function key is down
    }
}

/* 9000其他按键扫描函数 */
UINT8 ScanOthekey9000(void)
{
    volatile unsigned char tmp = 0x00;

    Key9000c = 0x00;

    printk("Start scanning\n");

    GPIO_SetKeyRows(0x06);
    tmp = GPIO_ReadKeyCols();

    printk("First read tmp = 0x%02X\n", tmp);

    if ((tmp & 0x06) == 0x06)
    {
        printk("No other key detected (tmp&0x06 == 0x06)\n");
        return FALSE;
    }

    printk("Key detected, debouncing...\n");
    k_msleep(100);
    GPIO_SetKeyRows(0x06);
    tmp = GPIO_ReadKeyCols();

    printk("Second read tmp = 0x%02X\n", tmp);

    if ((tmp & 0x06) == 0x06)
    {
        printk("Key released during debounce\n");
        return FALSE;
    }
    Key9000c = tmp & 0x06;
    printk("Key9000c after first scan = 0x%02X\n", Key9000c);

    GPIO_SetKeyRows(0xB0);
    tmp = GPIO_ReadKeyCols();

    printk("Third read tmp = 0x%02X\n", tmp);

    if ((tmp & 0xB0) == 0xB0)
    {
        printk("No other key detected in second scan (tmp&0xB0 == 0xB0)\n");
        return FALSE;
    }
    Key9000c |= (tmp & 0xB0);
    printk("Key9000c after second scan = 0x%02X\n", Key9000c);

    GPIO_SetKeyRows(0x0F);

    //Convert key value
    printk("Converting key value\n");
    switch (Key9000c)
    {
        case 0xA2:
            tmp = 0x4E;
            printk("Matched 0xA2 -> 0x4E\n");
            break;
        case 0x34:
            tmp = 0x5D;
            printk("Matched 0x34 -> 0x5D\n");
            break;
        case 0x94:
            tmp = 0x55;
            printk("Matched 0x94 -> 0x55\n");
            break;
        default:
            printk("No match found for Key9000c = 0x%02X\n", Key9000c);
            return FALSE;
            break;
    }

    Key9000c = tmp;
    printk("Final Key9000c = 0x%02X\n", Key9000c);

    return TRUE;
}

/* 键盘扫描和传输主函数 */
void Keyboard_Scan_And_Transmit(void)
{
    volatile unsigned int Key;

    if (TRUE == Key_Scan())
    {
        printk("Key detected in 9000 %X\n", gKey_Buffer);
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
                printk("Regular key, sending key codes: 0x%02X 0x%02X\n", (Key >> 8) & 0xff, Key & 0xff);
                uart_send_char((Key >> 8) & 0xff);
                uart_send_char(Key & 0xff);
            }

            uart_send_char((Key >> 8) & 0xff);
            uart_send_char(Key & 0xff);
        }
    }
    else if (TRUE == ScanOthekey9000())
    {
        printk("Other key detected in 9000, sending codes: 0xF0 0x%02X\n", Key9000c);
        uart_send_char(0xF0);
        uart_send_char(Key9000c);

        uart_send_char(0xF0);
        uart_send_char(Key9000c);
    }
}