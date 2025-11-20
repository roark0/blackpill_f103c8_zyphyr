# Key_Scan函数新旧实现关系对应表

## 1. 概述

本文档对比分析了原始keyboard.c和Zephyr版本src/keyboard_logic.c中Key_Scan函数的实现差异和对应关系。

## 2. 扫描方式对比

### 2.1 原始代码 (keyboard.c)
- **硬件操作方式**: 直接操作P1和P2端口寄存器
- **扫描策略**: 一次性扫描，先获取列状态，再获取行状态
- **去抖动方式**: 简单延时去抖动 (Delay(50))
- **检测方式**: 电平检测，检测到非全1状态即认为有按键按下

### 2.2 新代码 (src/keyboard_logic.c)
- **硬件操作方式**: 使用GPIO接口函数 (GPIO_SetKeyRows, GPIO_ReadKeyCols)
- **扫描策略**: 逐行列扫描 (6行×8列)，循环扫描每个行列交叉点
- **去抖动方式**: 边沿检测 + 二次确认 + 状态跟踪
- **检测方式**: 边沿触发检测，仅在按键状态从释放变为按下时响应

## 3. 硬件接口映射

| 原始代码 | 新代码 | 说明 |
|----------|--------|------|
| P1 (端口1) | PB8-PB13 (行) | 行扫描线 |
| P2 (端口2) | PA0-PA7 (列) | 列读取线 |
| P1=0x00, P2=0xFF | GPIO_SetKeyRows(row_pattern) | 设置行扫描模式 |
| P2=0x00, P1=0xFF | 读取列状态 | 读取列输入 |

## 4. 核心算法对比

### 4.1 原始代码算法
```c
// 1. 设置P1为0，P2为1，读取列状态
P1 = 0x00;
P2 = 0xFF;
Key_Code = P2;

// 2. 如果检测到按键，再次确认
if (Key_Code != 0xFF) {
    // 延时去抖动
    P2 = 0xFF;
    Key_Code = P2;
    
    // 3. 获取行状态
    P2 = 0x00;
    P1 = 0xFF;
    Key_Code = P1;
    
    // 4. 组合行列信息
    gKey_Buffer = (列状态 << 8) + 行状态;
}
```

### 4.2 新代码算法
```c
// 1. 逐行扫描
for (row = 0; row < 6; row++) {
    // 设置当前行低电平，其他行高电平
    uint8_t row_pattern = ~(1 << row) & 0x3F;
    GPIO_SetKeyRows(row_pattern);
    
    // 短暂延时稳定信号
    k_msleep(1);
    
    // 读取列状态
    tmp = GPIO_ReadKeyCols();
    
    // 2. 检查每列状态变化
    for (col = 0; col < 8; col++) {
        uint8_t current_state = (tmp & (1 << col)) ? 1 : 0;
        uint8_t previous_state = prev_key_state[row][col];
        
        // 3. 检测边沿触发 (0 -> 1)
        if (previous_state == 0 && current_state == 1) {
            // 延时并二次确认
            k_msleep(10);
            uint8_t tmp2 = GPIO_ReadKeyCols();
            uint8_t confirm_state = (tmp2 & (1 << col)) ? 1 : 0;
            
            if (confirm_state == 1) {
                // 生成键值
                gKey_Buffer = ((uint16_t)(1 << col) << 8) | (1 << row);
            }
        }
        
        // 更新状态数组
        prev_key_state[row][col] = current_state;
    }
}
```

## 5. 键值生成公式

| 代码版本 | 键值生成公式 | 说明 |
|----------|--------------|------|
| 原始代码 | `gKey_Buffer = (列状态 << 8) + 行状态` | 直接组合列和行的完整字节 |
| 新代码 | `gKey_Buffer = ((uint16_t)(1 << col) << 8) \| (1 << row)` | 组合行列位置的掩码 |

## 6. 全局变量映射

| 原始代码 | 新代码 | 类型差异 | 说明 |
|----------|--------|----------|------|
| `bit Flag_Key` | `uint8_t Flag_Key` | bit -> uint8_t | 标志位类型变化 |
| `unsigned int gKey_Buffer` | `volatile unsigned int gKey_Buffer` | 增加volatile | 增加volatile修饰符 |
| `unsigned int gFunc_Buffer` | `volatile unsigned int gFunc_Buffer` | 增加volatile | 增加volatile修饰符 |
| - | `static uint8_t prev_key_state[6][8]` | 新增 | 新增按键状态跟踪数组 |

## 7. 功能特性对比

| 特性 | 原始代码 | 新代码 | 说明 |
|------|----------|--------|------|
| 硬件抽象 | 直接寄存器操作 | GPIO接口函数 | 新代码更易移植 |
| 按键检测 | 电平检测 | 边沿检测 | 新代码避免重复触发 |
| 去抖动 | 简单延时 | 二次确认 | 新代码更可靠 |
| 状态跟踪 | 无 | 每按键跟踪 | 新代码更精确 |
| 扫描精度 | 一次性 | 逐点扫描 | 新代码可精确定位 |
| 并发处理 | 不支持 | 支持 | 新代码可处理多键同时按下 |

## 8. 关键差异总结

1. **硬件抽象**: 新代码使用GPIO接口函数，提高了代码的可移植性。
2. **检测精度**: 新代码使用逐行列扫描，可以精确定位每个按键。
3. **防重复触发**: 新代码使用边沿检测和状态跟踪，避免按键持续按下期间的重复处理。
4. **去抖动算法**: 新代码使用二次确认机制，提高检测可靠性。
5. **多键处理**: 新代码的算法天然支持多键同时按下情况。

## 9. 移植注意事项

1. 硬件初始化: 需要保证GPIO接口函数与原始P1/P2端口功能一致
2. 时序要求: 新代码中的延时参数需要与原始代码的时序匹配
3. 键值格式: 虽然生成算法不同，但最终键值应该保持一致以兼容后续处理
4. 状态跟踪: 新代码的状态数组需要正确初始化