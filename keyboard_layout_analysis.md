# 键盘布局分析文档

## 1. 概述

本文档记录了对 `keyboard.c` 文件中键盘扫描逻辑的分析结果，用于指导 `src/keyboard_logic.c` 的后续修改工作。

## 2. Key_Scan 函数分析

### 2.1 扫描原理
`Key_Scan` 函数使用行列扫描法检测按键状态：
- 首先将 P1（行）设为 0，P2（列）设为 1，读取 P2 获取列状态
- 如果检测到按键按下，将 P2（列）设为 0，P1（行）设为 1，读取 P1 获取行状态
- 最终 `gKey_Buffer` 格式为：`gKey_Buffer = (列状态 << 8) + 行状态`

### 2.2 Key 值结构
- **高字节**（高8位）：列信息（扫描时P2的值）
- **低字节**（低8位）：行信息（扫描时P1的值）
- 按键按下时对应位为低电平（0），未按下为高电平（1）

### 2.3 行列位置推断
通过计算补码（~byte）可以确定哪一位是低电平（按键被按下）：
- 例如：0xFDFD，高字节0xFD=11111101，补码0x02=00000010，第1位为低电平，对应第1列
- 低字节0xFD=11111101，补码0x02=00000010，第1位为低电平，对应第1行

## 3. 键盘布局（6行×8列）

### 3.1 完整布局表
| 行 | 列0 | 列1 | 列2 | 列3 | 列4 | 列5 | 列6 | 列7 |
|----|-----|-----|-----|-----|-----|-----|-----|-----|
| 0 | ... | Z | X | C | V | B | N | M |
| 1 | Capslock | A | S | D | F | G | H | J |
| 2 | TAB | Q | W | E | R | T | Y | U |
| 3 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| 4 | ... | 0 | K | Dot | Space | Enter | I | 9 |
| 5 | ... | ... | P | L | ... | ... | O | Backspace |

### 3.2 关键按键行列位置
以下是一些重要按键的行列位置映射：

| 按键 | 行 | 列 | 键值 | 对应的 Key_Decode case |
|------|----|----|------|----------------------|
| A | 1 | 1 | 0xFDFD | case 0xfdfd |
| Z | 0 | 1 | 0xFDFE | case 0xfdfe |
| 1 | 3 | 0 | 0xFEF7 | case 0xfef7 |
| 0 | 4 | 1 | 0xFDEF | case 0xfdef |
| Enter | 4 | 5 | 0xDFEF | case 0xdfef |
| Space | 4 | 4 | 0xEFEF | case 0xefef |
| TAB | 2 | 0 | 0xFEFB | case 0xfefb |
| Capslock | 1 | 0 | 0xFEFD | case 0xfefd |
| Q | 2 | 1 | 0xFDFB | case 0xfdfb |
| W | 2 | 2 | 0xFBFB | case 0xfbfb |
| E | 2 | 3 | 0xF7FB | case 0xf7fb |
| R | 2 | 4 | 0xEFFB | case 0xeffb |
| T | 2 | 5 | 0xDFFB | case 0xdffb |
| Y | 2 | 6 | 0xBFFB | case 0xbffb |
| U | 2 | 7 | 0x7FFB | case 0x7ffb |
| S | 1 | 2 | 0xFBFD | case 0xfbfd |
| D | 1 | 3 | 0xF7FD | case 0xf7fd |
| F | 1 | 4 | 0xEFFD | case 0xeffd |
| G | 1 | 5 | 0xDFFD | case 0xdffd |
| H | 1 | 6 | 0xBFFD | case 0xbffd |
| J | 1 | 7 | 0x7FFD | case 0x7ffd |
| K | 4 | 2 | 0xFBEF | case 0xfbef |
| L | 5 | 3 | 0xF7DF | case 0xf7df |
| I | 4 | 6 | 0xBFEF | case 0xbfef |
| O | 5 | 6 | 0xBFDF | case 0xbfdf |
| P | 5 | 2 | 0xFBDF | case 0xfbdf |
| 2 | 3 | 1 | 0xFDF7 | case 0xfdf7 |
| 3 | 3 | 2 | 0xFBF7 | case 0xfbf7 |
| 4 | 3 | 3 | 0xF7F7 | case 0xf7f7 |
| 5 | 3 | 4 | 0xEFF7 | case 0xeff7 |
| 6 | 3 | 5 | 0xDFF7 | case 0xdff7 |
| 7 | 3 | 6 | 0xBFF7 | case 0xbff7 |
| 8 | 3 | 7 | 0x7FF7 | case 0x7ff7 |
| 9 | 4 | 7 | 0x7FEF | case 0x7fef |
| Shift | 0 | 0 | 0xFEFE | Function_Key_Decode case 0xFEFE |
| LCtrl | 4 | 0 | 0xFEEF | Function_Key_Decode case 0xFEEF |

## 4. Key_Decode 函数映射

### 4.1 扫描码到键码的映射
Key_Decode 函数将扫描码（来自 Key_Scan）映射到键盘码：
- 高字节：断码（breakcode）
- 低字节：通码（makecode）

### 4.2 示例映射
| 扫描码 | 对应按键 | 键码输出 |
|--------|----------|----------|
| 0xFDEF | 0 | 0xF045 |
| 0xFEF7 | 1 | 0xF016 |
| 0xFDFD | A | 0xF01C |

## 5. Function_Key_Decode 函数分析

### 5.1 功能键定义
Function_Key_Decode 函数处理特殊功能键（如Shift、Ctrl）的按下和组合键逻辑：

| 功能键 | 键值 | 行 | 列 | 作用 |
|--------|------|----|----|------|
| Shift | 0xFEFE | 0 | 0 | 左移键，用于字符大小写转换等 |
| LCtrl | 0xFEEF | 4 | 0 | 左控制键，用于组合快捷键 |

### 5.2 功能键处理逻辑
- **Shift键处理**：当检测到0xFEFE键值时，进入循环等待其他按键按下，然后生成0xF012键码（Shift的断码/通码）
- **LCtrl键处理**：当检测到0xFEEF键值时，进入循环等待其他按键按下，然后生成0xF014键码（Ctrl的断码/通码）
- **组合键处理**：当功能键按下时，如果同时按下其他键，会修改gKey_Buffer以表示组合键状态

### 5.3 功能键代码逻辑
- 检测到功能键后，进入while循环持续扫描其他按键
- 如果没有其他按键按下且功能键释放，则退出循环
- 如果检测到其他按键按下，会根据行列位置进行组合键处理
- 设置gFunc_Buffer为功能键的键码（Shift为0xF012，Ctrl为0xF014）
- 设置Flag_Updown标志为1，表示功能键被按下

## 7. 修改 src/keyboard_logic.c 的指导原则

### 5.1 保持行列扫描逻辑一致
- 确保 `Key_Scan` 函数的行列扫描逻辑与原始 `keyboard.c` 保持一致
- 高字节存储列信息，低字节存储行信息

### 5.2 保持键码映射一致
- 确保 `Key_Decode` 函数的扫描码到键码映射与原始版本一致
- 维持相同的断码/通码格式

### 5.3 保持功能键处理逻辑一致
- Function_Key_Decode 函数的处理逻辑应保持一致
- 特别注意功能键（Shift、Ctrl）的处理方式

## 8. 注意事项

1. 在修改 `src/keyboard_logic.c` 时，确保行列扫描的逻辑与这个分析文档一致
2. 保持原始的键码映射关系，以确保键盘功能正常
3. 特别注意功能键的处理，避免破坏原有的组合键功能
4. 测试时应验证所有按键的位置映射是否正确
