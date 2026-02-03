# rt-a19_V2.0 - 实时温度控制系统

这是一个基于 Zephyr RTOS 和 QPC (Quantum Leaps Platform Components) 框架的实时温度控制系统。

## 项目概述

该系统使用 DS18B20 温度传感器监测温度，并通过 PID 算法控制加热器，以维持预设的温度值。系统使用 QPC 活动对象模式实现事件驱动的实时控制。

## 系统特性

- 使用 QPC 框架实现活动对象模式
- 实时温度监测与控制
- PID 控制算法
- 按键控制温度档位切换
- LED 指示当前温度档位
- 温度显示功能
- 拨码开关设置温度偏移量

## 组件说明

### 硬件组件
- DS18B20 温度传感器：用于温度检测
- 加热器：被控对象
- 按键：用于切换温度设定值
- LED：指示当前温度档位
- 拨码开关：设置温度偏移量

### 软件组件
- **温度采样活动对象**：实现温度采样和发布温度更新事件
- **温度控制活动对象**：实现温度控制逻辑和 PID 算法
- **PID 控制器**：实现 PID 算法
- **显示模块**：处理温度显示
- **开关模块**：处理拨码开关和按键输入
- **LED 模块**：控制 LED 指示灯

## 功能特性

- **温度采样**：每秒采样一次温度值（通过独立的温度采样 AO）
- **PID 控制**：基于当前温度与设定值的差异控制加热器
- **档位切换**：通过按键切换预设的 4 个温度档位
- **事件驱动**：采用 QPC 消息传递机制实现模块间解耦
- **显示更新**：定时更新温度显示
- **加热控制**：实现 PWM 式加热控制算法
- **温度偏移**：通过拨码开关动态调整温度偏移值

## 项目结构

```
.
├── include/          # 头文件
│   ├── bsp.h         # 硬件抽象层定义
│   ├── ds18b20.h     # DS18B20 驱动接口
│   ├── temperature_control.h  # 温度控制 AO 接口
│   ├── temperature_sampling.h # 温度采样 AO 接口
│   ├── pid.h         # PID 控制器接口
│   ├── display.h     # 显示模块接口
│   ├── led.h         # LED 控制接口
│   └── switch.h      # 开关控制接口
├── src/              # 源代码
│   ├── main.c        # 主程序入口
│   ├── ds18b20.c     # DS18B20 驱动实现
│   ├── temperature_control.c   # 温度控制 AO 实现
│   ├── temperature_sampling.c  # 温度采样 AO 实现
│   ├── pid.c         # PID 控制器实现
│   ├── display.c     # 显示模块实现
│   ├── led.c         # LED 控制实现
│   └── switch.c      # 开关控制实现
├── boards/           # 板级支持包
├── third_party/      # 第三方库 (QPC)
├── CMakeLists.txt    # CMake 构建配置
└── prj.conf          # 项目配置文件
```

## 构建说明

```bash
# 初始化构建
source ~/zephyrproject/.venv/bin/activate && source ~/zephyrproject/zephyr/zephyr-env.sh
west build -b stm32_min_dev -p auto
```

## 运行说明

1. 确保硬件连接正确（DS18B20 传感器、加热器、按键、LED、显示模块等）
2. 构建并烧录固件到目标板
3. 系统启动后会自动开始温度监测和控制
4. 通过按键切换温度档位（4 个预设温度值）
5. LED 指示当前选中的温度档位

## 信号说明

- `TEMP_UPDATE_SIG`：温度更新通知
- `DISPLAY_UPDATE_SIG`：显示更新通知
- `TEMP_TIMEOUT_SIG`：温度采样定时器超时
- `DISPLAY_TIMEOUT_SIG`：显示刷新定时器超时
- `BUTTON_PRESSED_SIG`：按键按下信号
- `HEAT_CONTROL_SIG`：加热控制信号

## 架构设计

### 活动对象 (Active Objects)

系统采用双活动对象架构：

1. **温度采样活动对象 (TempSampler)**：
   - 负责定期读取 DS18B20 温度传感器
   - 通过工作队列异步执行耗时的温度读取操作
   - 发布温度更新事件 (TEMP_UPDATE_SIG) 到系统

2. **温度控制活动对象 (TempCtrl)**：
   - 订阅温度更新事件
   - 实现 PID 控制算法
   - 控制加热器输出
   - 处理用户输入（按键）
   - 更新显示内容

### 消息传递

- 温度采样 AO 发布 `TEMP_UPDATE_SIG` 事件
- 温度控制 AO 订阅并处理温度更新事件
- 按键事件通过 `BUTTON_PRESSED_SIG` 信号传递
- 定时器事件用于定期执行任务（显示更新、加热控制等）

## 维护记录

- 分离温度采样和温度控制为独立的活动对象
- 简化 DS18B20 接口，移除温度偏移参数
- 实现事件驱动的架构，提高模块间解耦
- 优化 PID 控制算法
- 清理了未使用的 SETPOINT_CHANGE_SIG 信号
- 优化了温度采样工作队列实现
- 改进了 PID 控制算法的响应性