#ifndef PID_H
#define PID_H

/**
 * @brief PID 控制器结构体
 */
typedef struct {
    float kp;           // 比例系数
    float ki;           // 积分系数
    float kd;           // 微分系数
    float setpoint;     // 设定值
    float integral;     // 积分项
    float prev_error;   // 上一次误差
    float output_min;   // 输出最小值
    float output_max;   // 输出最大值
} pid_controller_t;

/**
 * @brief 初始化 PID 控制器
 * @param pid PID 控制器指针
 * @param kp 比例系数
 * @param ki 积分系数
 * @param kd 微分系数
 * @param setpoint 设定值
 * @param output_min 输出最小值
 * @param output_max 输出最大值
 */
void pid_init(pid_controller_t *pid, float kp, float ki, float kd, float setpoint, float output_min, float output_max);

/**
 * @brief 计算 PID 输出
 * @param pid PID 控制器指针
 * @param measurement 当前测量值
 * @return PID 输出值
 */
float pid_compute(pid_controller_t *pid, float measurement);

/**
 * @brief 设置 PID 设定值
 * @param pid PID 控制器指针
 * @param setpoint 设定值
 */
void pid_set_setpoint(pid_controller_t *pid, float setpoint);

/**
 * @brief 重置 PID 控制器
 * @param pid PID 控制器指针
 */
void pid_reset(pid_controller_t *pid);

#endif /* PID_H */