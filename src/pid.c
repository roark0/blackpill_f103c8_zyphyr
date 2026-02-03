/**
 * @file pid.c
 * @brief PID 控制器实现
 * 
 * 该文件实现了 PID (比例-积分-微分) 控制器算法，用于温度控制等闭环控制系统。
 */

#include "pid.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pid, LOG_LEVEL_INF);

/**
 * @brief 初始化 PID 控制器
 * 
 * 使用指定的参数初始化 PID 控制器结构体。
 * 
 * @param pid 指向 PID 控制器结构体的指针
 * @param kp 比例系数
 * @param ki 积分系数
 * @param kd 微分系数
 * @param setpoint 设定值（目标值）
 * @param output_min 输出最小值限制
 * @param output_max 输出最大值限制
 */
void pid_init(pid_controller_t *pid, float kp, float ki, float kd, float setpoint, float output_min, float output_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = setpoint;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output_min = output_min;
    pid->output_max = output_max;
}

/**
 * @brief 计算 PID 输出
 * 
 * 根据当前测量值计算 PID 控制器的输出值。
 * 
 * @param pid 指向 PID 控制器结构体的指针
 * @param measurement 当前测量值
 * @return float PID 控制器输出值
 */
float pid_compute(pid_controller_t *pid, float measurement)
{
    float error = pid->setpoint - measurement;
    float p_out, i_out, d_out, output;

    // 比例项
    p_out = pid->kp * error;

    // 积分项（带抗饱和）
    pid->integral += error;
    if (pid->integral > pid->output_max / pid->ki)
    {
        pid->integral = pid->output_max / pid->ki;
    }
    else if (pid->integral < pid->output_min / pid->ki)
    {
        pid->integral = pid->output_min / pid->ki;
    }
    i_out = pid->ki * pid->integral;

    // 微分项
    d_out = pid->kd * (error - pid->prev_error);
    pid->prev_error = error;

    // 总输出
    output = p_out + i_out + d_out;

    // 输出限幅
    if (output > pid->output_max)
    {
        output = pid->output_max;
    }
    else if (output < pid->output_min)
    {
        output = pid->output_min;
    }

    return output;
}

/**
 * @brief 设置 PID 设定值
 * 
 * 更新 PID 控制器的设定值（目标值）。
 * 
 * @param pid 指向 PID 控制器结构体的指针
 * @param setpoint 新的设定值（目标值）
 */
void pid_set_setpoint(pid_controller_t *pid, float setpoint)
{
    pid->setpoint = setpoint;
    pid->integral = 0.0f;  // 设定值改变时重置积分项
}

/**
 * @brief 重置 PID 控制器
 * 
 * 重置 PID 控制器的积分项和前一误差值。
 * 
 * @param pid 指向 PID 控制器结构体的指针
 */
void pid_reset(pid_controller_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}