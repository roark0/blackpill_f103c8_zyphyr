/**
 * @file bsp.h
 * @brief Board Support Package for rt-a19_V2.0 project
 * 
 * This file provides board support package definitions for the rt-a19_V2.0
 * project, including QPC tick configuration, signal definitions, and
 * error handling interfaces.
 */

#ifndef BSP_H
#define BSP_H

#include <zephyr/kernel.h>
#include "qpc.h"

#ifdef __cplusplus
extern "C"
{
#endif

//!< QPC tick 配置 - QPC tick 间隔（毫秒）
#define QPC_TICK_MS 10 

/**
 * @brief 将毫秒转换为 QPC tick 数
 * @note 公式：ms / QPC_TICK_MS
 * @example MS2QPC(500) = 500 / 10 = 50 tick
 * @param ms 毫秒数
 * @return 对应的 QPC tick 数
 */
#define MS2QPC(ms) ((ms) / QPC_TICK_MS)

/**
 * @brief 应用程序信号枚举
 * 
 * 定义应用程序中使用的所有 QPC 信号，包括发布信号和投递信号。
 */
enum AppSignals {
    /* Published Signals - 用于发布订阅模式 */
    TEMP_UPDATE_SIG = Q_USER_SIG,    //!< 温度更新通知
    DISPLAY_UPDATE_SIG,              //!< 显示更新通知

    /* Posted Signals - 用于直接投递到特定 AO */
    TEMP_TIMEOUT_SIG,                //!< 温度采样定时器超时
    DISPLAY_TIMEOUT_SIG,             //!< 显示刷新定时器超时
    BUTTON_PRESSED_SIG,              //!< 按键按下信号
    HEAT_CONTROL_SIG,                //!< 加热控制信号

    MAX_SIG                          //!< 信号枚举最大值标记
};

/**
 * @brief QPC 错误处理函数
 * 
 * 当 QPC 框架中发生错误时调用此函数，提供错误的模块名和 ID。
 * 
 * @param module 发生错误的模块名称
 * @param id 错误 ID
 */
Q_NORETURN Q_onError(char const *const module, int const id);

/**
 * @brief QPC 时钟滴答处理函数
 * 
 * 处理 QPC 时钟滴答事件，通常用于驱动定时器和时间事件。
 */
void QF_onClockTick(void);

/**
 * @brief QPC 启动处理函数
 * 
 * QPC 框架启动时调用此函数，用于执行必要的初始化任务。
 */
void QF_onStartup(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */