#ifndef __GPIO_CONTROL_H
#define __GPIO_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 函数声明 */
void GPIO_Control_Init(void);

// 键盘行控制函数
void GPIO_SetKeyRows(uint8_t row_pattern);
void GPIO_SetKeyRow(uint8_t row, uint8_t state);

// 键盘列读取函数
uint8_t GPIO_ReadKeyCols(void);
uint8_t GPIO_ReadKeyCol(uint8_t col);

// LED 控制函数
void GPIO_SetLED(uint8_t led_num, uint8_t state);
void GPIO_ToggleLED(uint8_t led_num);

// CAPSLOCK 控制函数
void GPIO_SetCapsLock(uint8_t state);
void GPIO_ToggleCapsLock(void);
uint8_t GPIO_ReadCapsLock(void);

// 键盘扫描函数
uint16_t GPIO_ScanKeyboard(void);

// 特殊按键控制函数
void GPIO_SetSpecialKeyOutput(uint8_t key_type, uint8_t state);
uint8_t GPIO_ReadSpecialKeyInput(uint8_t key_type);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_CONTROL_H */