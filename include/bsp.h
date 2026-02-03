/**
 * @file bsp.h
 * @brief Board Support Package for rt-a19_V2.0 project
 */

#ifndef BSP_H
#define BSP_H

#include <zephyr/kernel.h>
#include "qpc.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* QPC tick 配置 */
#define QPC_TICK_MS 10 /* QPC tick 间隔（毫秒） */

/**
 * @brief 将毫秒转换为 QPC tick 数
 * @note 公式：ms / QPC_TICK_MS
 * @example MS2QPC(500) = 500 / 10 = 50 tick
 */
#define MS2QPC(ms) ((ms) / QPC_TICK_MS)

/* ============================================================
 * QPC 信号定义
 * ============================================================ */
enum AppSignals {
    /* ==================== Published Signals ==================== */
    TEMP_UPDATE_SIG = Q_USER_SIG,       /* 温度更新通知 */
    DISPLAY_UPDATE_SIG,                 /* 显示更新通知 */

    /* ==================== Posted Signals ==================== */
    TEMP_TIMEOUT_SIG,                   /* 温度采样定时器超时 */
    DISPLAY_TIMEOUT_SIG,                /* 显示刷新定时器超时 */
    BUTTON_PRESSED_SIG,                 /* 按键按下信号 */
    SETPOINT_CHANGE_SIG,                /* 设定值改变信号 */

    MAX_SIG
};

    /**
 * @brief QPC Error Handler
 * @param module Module name where error occurred
 * @param id Error ID
 */
    Q_NORETURN Q_onError(char const *const module, int const id);

    /**
 * @brief QPC Tick Handler
 */
    void QF_onClockTick(void);

    /**
 * @brief QPC Startup Handler
 */
    void QF_onStartup(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */