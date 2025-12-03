#ifndef __KEYBOARD_LOGIC_H
#define __KEYBOARD_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>



/* 宏定义 */
#define SCANCODESET_1    1
#define SCANCODESET_2    2
#define SCANCODESET_3    3

#define SCANCODESET   SCANCODESET_2

#define MACHINE_1904C   0x10
#define MACHINE_9000    0x13
// HOSTMACHINE 定义已移至 timer.h

#define TRUE      1
#define FALSE     0

#define UINT8     uint8_t
#define UINT16    uint16_t

#define TRANSDELAY  20

#define MAX9000SCAN   3
#define MAX1904SCAN   3

/* 全局变量声明 */
extern volatile unsigned int gKey_Buffer;        //common keycode buffer
extern volatile unsigned int gFunc_Buffer;       //function keycode buffer
extern volatile uint8_t Flag_Updown;             //the flag of function key keeping down or not
extern volatile uint8_t Flag_Key;                //the flag of any key keeping down or not

extern volatile uint8_t Key9000c;

extern volatile char G_uc9000ScanInterval;

UINT8 Key_Scan(void);
unsigned int Key_Decode(unsigned int key);
void Function_Key_Decode(unsigned int Func_Key);
UINT8 ScanSpecialKeys(void);
void Keyboard_Scan_And_Transmit(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEYBOARD_LOGIC_H */