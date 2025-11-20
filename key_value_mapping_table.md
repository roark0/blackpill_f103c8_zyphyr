# Key值对应表

## 1. 概述

本文档详细列出了原始keyboard.c和Zephyr版本src/keyboard_logic.c中按键值的对应关系。由于两个版本的扫描方式不同，但需要保持键值一致以确保功能正常，所以需要建立完整的键值映射表。

## 2. 键值生成原理

### 2.1 原始代码 (keyboard.c) 键值生成
- 高字节：列状态（P2值）
- 低字节：行状态（P1值）
- 原理：按键按下时对应位为低电平(0)，未按下为高电平(1)

### 2.2 新代码 (src/keyboard_logic.c) 键值生成
- 高字节：列掩码 (1 << col)
- 低字节：行掩码 (1 << row)
- 原理：通过行列位置计算掩码

## 3. 完整键值映射表 (按行/列顺序排列)

### 3.1 行 0 (row=0)
| 按键 | 行 | 列 | 原始代码键值 | 新代码键值 | Key_Decode映射 | 说明 |
|------|----|----|---------------|------------|----------------|------|
| Shift | 0 | 0 | 0xFEFE | 0x0101 | case 0xFEFE: Function_Key_Decode | 功能键 |
| Z | 0 | 1 | 0xFDFE | 0x0201 | case 0xfdfe: iKeyID = 0xF01A | |
| X | 0 | 2 | 0xFBFE | 0x0401 | case 0xfbfe: iKeyID = 0xF022 | |
| C | 0 | 3 | 0xF7FE | 0x0801 | case 0xf7fe: iKeyID = 0xF021 | |
| V | 0 | 4 | 0xEFFE | 0x1001 | case 0xeffe: iKeyID = 0xF02A | |
| B | 0 | 5 | 0xDFFE | 0x2001 | case 0xdffe: iKeyID = 0xF032 | |
| N | 0 | 6 | 0xBFFE | 0x4001 | case 0xbffe: iKeyID = 0xF031 | |
| M | 0 | 7 | 0x7FFE | 0x8001 | case 0x7ffe: iKeyID = 0xF03A | |

### 3.2 行 1 (row=1)
| 按键 | 行 | 列 | 原始代码键值 | 新代码键值 | Key_Decode映射 | 说明 |
|------|----|----|---------------|------------|----------------|------|
| Capslock | 1 | 0 | 0xFEFD | 0x0102 | case 0xfefd: iKeyID = 0xF058 | |
| A | 1 | 1 | 0xFDFD | 0x0202 | case 0xfdfd: iKeyID = 0xF01C | |
| S | 1 | 2 | 0xFBFD | 0x0402 | case 0xfbfd: iKeyID = 0xF01B | |
| D | 1 | 3 | 0xF7FD | 0x0802 | case 0xf7fd: iKeyID = 0xF023 | |
| F | 1 | 4 | 0xEFFD | 0x1002 | case 0xeffd: iKeyID = 0xF02B | |
| G | 1 | 5 | 0xDFFD | 0x2002 | case 0xdffd: iKeyID = 0xF034 | |
| H | 1 | 6 | 0xBFFD | 0x4002 | case 0xbffd: iKeyID = 0xF033 | |
| J | 1 | 7 | 0x7FFD | 0x8002 | case 0x7ffd: iKeyID = 0xF03B | |

### 3.3 行 2 (row=2)
| 按键 | 行 | 列 | 原始代码键值 | 新代码键值 | Key_Decode映射 | 说明 |
|------|----|----|---------------|------------|----------------|------|
| TAB | 2 | 0 | 0xFEFB | 0x0104 | case 0xfefb: iKeyID = 0xF00D | |
| Q | 2 | 1 | 0xFDFB | 0x0204 | case 0xfdfb: iKeyID = 0xF015 | |
| W | 2 | 2 | 0xFBFB | 0x0404 | case 0xfbfb: iKeyID = 0xF01D | |
| E | 2 | 3 | 0xF7FB | 0x0804 | case 0xf7fb: iKeyID = 0xF024 | |
| R | 2 | 4 | 0xEFFB | 0x1004 | case 0xeffb: iKeyID = 0xF02D | |
| T | 2 | 5 | 0xDFFB | 0x2004 | case 0xdffb: iKeyID = 0xF02C | |
| Y | 2 | 6 | 0xBFFB | 0x4004 | case 0xbffb: iKeyID = 0xF035 | |
| U | 2 | 7 | 0x7FFB | 0x8004 | case 0x7ffb: iKeyID = 0xF03C | |

### 3.4 行 3 (row=3)
| 按键 | 行 | 列 | 原始代码键值 | 新代码键值 | Key_Decode映射 | 说明 |
|------|----|----|---------------|------------|----------------|------|
| 1 | 3 | 0 | 0xFEF7 | 0x0108 | case 0xfef7: iKeyID = 0xF016 | |
| 2 | 3 | 1 | 0xFDF7 | 0x0208 | case 0xfdf7: iKeyID = 0xF01E | |
| 3 | 3 | 2 | 0xFBF7 | 0x0408 | case 0xfbf7: iKeyID = 0xF026 | |
| 4 | 3 | 3 | 0xF7F7 | 0x0808 | case 0xf7f7: iKeyID = 0xF025 | |
| 5 | 3 | 4 | 0xEFF7 | 0x1008 | case 0xeff7: iKeyID = 0xF02E | |
| 6 | 3 | 5 | 0xDFF7 | 0x2008 | case 0xdff7: iKeyID = 0xF036 | |
| 7 | 3 | 6 | 0xBFF7 | 0x4008 | case 0xbff7: iKeyID = 0xF03D | |
| 8 | 3 | 7 | 0x7FF7 | 0x8008 | case 0x7ff7: iKeyID = 0xF03E | |

### 3.5 行 4 (row=4)
| 按键 | 行 | 列 | 原始代码键值 | 新代码键值 | Key_Decode映射 | 说明 |
|------|----|----|---------------|------------|----------------|------|
| LCtrl | 4 | 0 | 0xFEEF | 0x0110 | case 0xFEEF: Function_Key_Decode | 功能键 |
| 0 | 4 | 1 | 0xFDEF | 0x0210 | case 0xfdef: iKeyID = 0xF045 | |
| K | 4 | 2 | 0xFBEF | 0x0410 | case 0xfbef: iKeyID = 0xF042 | |
| Dot | 4 | 3 | 0xF7EF | 0x0810 | case 0xf7ef: iKeyID = 0xF049 | |
| Space | 4 | 4 | 0xEFEF | 0x1010 | case 0xefef: iKeyID = 0xF029 | |
| Enter | 4 | 5 | 0xDFEF | 0x2010 | case 0xdfef: iKeyID = 0xF05A | |
| I | 4 | 6 | 0xBFEF | 0x4010 | case 0xbfef: iKeyID = 0xF043 | |
| 9 | 4 | 7 | 0x7FEF | 0x8010 | case 0x7fef: iKeyID = 0xF046 | |

### 3.6 行 5 (row=5)
| 按键 | 行 | 列 | 原始代码键值 | 新代码键值 | Key_Decode映射 | 说明 |
|------|----|----|---------------|------------|----------------|------|
| [空] | 5 | 0 | 0xFFFF | 0x0120 | - | 无按键 |
| [空] | 5 | 1 | 0xFFFF | 0x0220 | - | 无按键 |
| P | 5 | 2 | 0xFBDF | 0x0420 | case 0xfbdf: iKeyID = 0xF04D | |
| L | 5 | 3 | 0xF7DF | 0x0820 | case 0xf7df: iKeyID = 0xF04B | |
| [空] | 5 | 4 | 0xFFFF | 0x1020 | - | 无按键 |
| [空] | 5 | 5 | 0xFFFF | 0x2020 | - | 无按键 |
| O | 5 | 6 | 0xBFDF | 0x4020 | case 0xbfdf: iKeyID = 0xF044 | |
| Backspace | 5 | 7 | 0x7FDF | 0x8020 | case 0x7fdf: iKeyID = 0xF066 | |

## 4. 键值转换公式

### 4.1 从原始代码键值到行列位置
```c
// 原始代码键值格式：0xABCD (AB=列状态，CD=行状态)
uint8_t col_status = (key >> 8) & 0xFF;  // 高字节
uint8_t row_status = key & 0xFF;         // 低字节

// 计算行列位置 (低电平位为按下键)
uint8_t row = -1, col = -1;
for (int i = 0; i < 8; i++) {
    if (!((col_status >> i) & 1)) {  // 第i位为0 (低电平)
        col = i;
    }
    if (!((row_status >> i) & 1)) {  // 第i位为0 (低电平)
        row = i;
    }
}
```

### 4.2 从行列位置到新代码键值
```c
// 新代码键值格式：((1 << col) << 8) | (1 << row)
uint16_t new_key = ((uint16_t)(1 << col) << 8) | (1 << row);
```

## 5. 键值一致性验证

为了确保新旧代码的键值一致性，需要验证以下几点：

1. 每个按键在相同行列位置产生相同的原始键值
2. Key_Decode函数中的case语句能够正确处理所有键值
3. 功能键(Shift, LCtrl)的处理保持一致
4. 组合键的处理逻辑正确

## 6. 实现建议

在src/keyboard_logic.c中，为保持与原始代码的兼容性，需要：

1. 确保Key_Scan函数生成的键值与原始代码相同
2. 保持Key_Decode函数中的case映射不变
3. 确保Function_Key_Decode函数的处理一致

注意：由于在优化src/keyboard_logic.c时已经添加了键值转换函数，现在Key_Scan函数输出的键值将与旧版keyboard.c保持一致。