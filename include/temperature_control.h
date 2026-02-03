/**
 * @file temperature_control.h
 * @brief Temperature Control Active Object
 */

#ifndef TEMPERATURE_CONTROL_H
#define TEMPERATURE_CONTROL_H

#include "qpc.h"
#include "bsp.h"
#include "pid.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* 温度设定值档位 */
#define TEMP_SETPOINT_COUNT 4

/* 温度采样工作队列配置 */
#define TEMP_WORK_STACK_SIZE 1024
#define TEMP_WORK_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

/* Active Object 配置 */
#define TEMPCTRL_AO_STACK_SIZE 2048
#define TEMPCTRL_AO_PRIORITY 8

/* 按键按下事件 */
typedef struct {
    QEvt super;
} ButtonPressEvent;

/* PID 控制参数 */
typedef struct {
    float kp;
    float ki;
    float kd;
    float setpoint;
    float output_min;
    float output_max;
} PidParams;

/* 温度控制 Active Object */
typedef struct {
    QActive super;

    /* 定时器 */
    QTimeEvt timeEvtTemp;   /* 温度采样定时器 */
    QTimeEvt timeEvtDisp;   /* 显示刷新定时器 */

    /* 温度采样工作队列（用于异步执行 DS18B20 读取） */
    struct k_work temp_work;
    struct k_work_q temp_work_q;

    /* 温度数据 */
    float current_temp;
    float display_temp;
    float last_display_temp;
    float temp_offset;

    /* PID 控制器 */
    pid_controller_t pid;
    PidParams pid_params;

    /* 温度设定值 */
    float setpoints[TEMP_SETPOINT_COUNT];
    uint8_t current_setpoint_idx;

    /* 统计数据 */
    uint32_t sample_count;

} TempCtrl;

/**
 * @brief 构造温度控制 Active Object
 */
void TempCtrl_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_CONTROL_H */