/**
 * @file temperature_control.h
 * @brief Temperature Control Active Object
 * 
 * This file defines the interface for the Temperature Control Active Object,
 * which manages temperature control using PID algorithm, controls heating,
 * and manages display updates.
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

//!< 温度设定值档位数量
#define TEMP_SETPOINT_COUNT 4

//!< Active Object 配置 - 栈大小
#define TEMPCTRL_AO_STACK_SIZE 2048
//!< Active Object 配置 - 优先级
#define TEMPCTRL_AO_PRIORITY 8

/**
 * @brief 按键按下事件结构体
 * 
 * 定义了按键按下事件的数据结构
 */
typedef struct {
    QEvt super;  //!< QPC 事件基类
} ButtonPressEvent;

/**
 * @brief PID 控制参数结构体
 * 
 * 包含 PID 控制器的所有参数配置
 */
typedef struct {
    float kp;          //!< 比例系数
    float ki;          //!< 积分系数
    float kd;          //!< 微分系数
    float setpoint;    //!< 设定点值
    float output_min;  //!< 输出最小值
    float output_max;  //!< 输出最大值
} PidParams;

/**
 * @brief 温度控制 Active Object 结构体
 * 
 * 定义了温度控制 Active Object 的数据结构，包括 PID 控制器、
 * 温度数据、定时器和其他控制参数。
 */
typedef struct {
    QActive super;     //!< QPC Active Object 基类

    /* 定时器 */
    QTimeEvt timeEvtDisp;   //!< 显示刷新定时器
    QTimeEvt timeEvtHeat;   //!< 加热控制定时器 (100ms周期)

    /* 温度数据 */
    float current_temp;        //!< 当前温度值
    float display_temp;        //!< 显示温度值
    float last_display_temp;   //!< 上一次显示温度值
    float temp_offset;         //!< 温度偏移值

    /* PID 控制器 */
    pid_controller_t pid;      //!< PID 控制器实例
    PidParams pid_params;      //!< PID 控制参数

    /* 温度设定值 */
    float setpoints[TEMP_SETPOINT_COUNT];  //!< 温度设定值数组
    uint8_t current_setpoint_idx;          //!< 当前设定值索引

    /* 统计数据 */
    uint32_t sample_count;     //!< 采样计数

    /* 加热控制相关变量 */
    float pid_output;          //!< 最新的PID输出值
    uint8_t heat_cycle_pos;    //!< 当前加热周期位置 (0-9)
    uint8_t heat_on_count;     //!< 当前周期内加热的次数

} TempCtrl;

/**
 * @brief 构造温度控制 Active Object
 * 
 * 初始化温度控制 Active Object，设置初始参数和启动 AO。
 */
void TempCtrl_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_CONTROL_H */