/**
 * @file temperature_control.c
 * @brief Temperature Control Active Object implementation
 * 
 * This file implements the Temperature Control Active Object, which manages
 * temperature control using PID algorithm, controls heating elements,
 * and manages display updates based on temperature readings.
 */

#include "temperature_control.h"
#include "temperature_sampling.h"  //!< 包含温度采样AO的头文件
#include "led.h"
#include "display.h"
#include "switch.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tempctrl, LOG_LEVEL_INF);

//!< GPIO 设备节点 - 加热器
#define HEATER_NODE DT_ALIAS(heater)
static const struct gpio_dt_spec heater = GPIO_DT_SPEC_GET(HEATER_NODE, gpios);

//!< 温度控制 AO 实例（导出供其他模块使用）
TempCtrl l_tempCtrl;

//!< Active Object 线程栈
K_THREAD_STACK_DEFINE(tempctrl_ao_stack, TEMPCTRL_AO_STACK_SIZE);

//!< Active Object 事件队列（50个事件）
static QEvtPtr tempctrlQueueSto[50];

/* AO 状态转换声明 */
QState TempCtrl_initial(TempCtrl *const me, QEvt const *const e);
QState TempCtrl_running(TempCtrl *const me, QEvt const *const e);

//!< 显示刷新间隔（毫秒）
#define DISPLAY_REFRESH_INTERVAL_MS 1000 
//!< 加热控制间隔（毫秒）
#define HEAT_CONTROL_INTERVAL_MS 100     

//!< 显示滤波参数 - 目标值权重
#define DISPLAY_TARGET_TIMES 0
//!< 显示滤波参数 - 历史值权重
#define DISPLAY_LAST_TIMES 0

/**
 * @brief 温度控制 AO 构造函数
 * 
 * 初始化温度控制 Active Object，设置初始参数和启动 AO。
 * 该函数会初始化 PID 控制器、设置初始设定值、启动定时器并订阅相关事件。
 */
void TempCtrl_ctor(void)
{
    TempCtrl *me = &l_tempCtrl;
    LOG_INF("TempCtrl_ctor: Starting");

    LOG_DBG("TempCtrl_ctor: Constructing QActive");
    QActive_ctor(&me->super, Q_STATE_CAST(&TempCtrl_initial));

    /* 构造时间事件 */
    QTimeEvt_ctorX(&me->timeEvtDisp, &me->super, DISPLAY_TIMEOUT_SIG, 0U);
    QTimeEvt_ctorX(&me->timeEvtHeat, &me->super, HEAT_CONTROL_SIG, 0U); /* 加热控制定时器 */

    /* 初始化温度数据 */
    me->current_temp         = 0.0f;
    me->display_temp         = 0.0f;
    me->last_display_temp    = 37.0f;
    me->sample_count         = 0;
    me->current_setpoint_idx = 2;
    me->temp_offset          = 0.0f;

    /* 初始化温度设定值 */
    me->setpoints[0] = 25.0f;
    me->setpoints[1] = 30.0f;
    me->setpoints[2] = 37.0f;
    me->setpoints[3] = 45.0f;

    /* 初始化 PID 控制器参数 */
    me->pid_params.kp         = 10.0f; /* 提高比例系数，提供更强的响应以达到设定值 */
    me->pid_params.ki         = 0.0f;  /* 稍微增加积分系数，帮助消除稳态误差 */
    me->pid_params.kd         = 0.0f;  /* 保持适中的微分系数，抑制可能的振荡 */
    me->pid_params.setpoint   = me->setpoints[2];
    me->pid_params.output_min = 0.0f;
    me->pid_params.output_max = 10.0f;

    /* 初始化 PID 控制器 */
    pid_init(&me->pid, me->pid_params.kp, me->pid_params.ki, me->pid_params.kd, me->pid_params.setpoint, me->pid_params.output_min,
             me->pid_params.output_max);

    /* 初始化加热控制变量 */
    me->pid_output     = 0.0f;
    me->heat_cycle_pos = 0;
    me->heat_on_count  = 0;

    /* 启动活动对象 */
    LOG_DBG("TempCtrl_ctor: Starting Active Object");
    QActive_start(&me->super, Q_PRIO(TEMPCTRL_AO_PRIORITY, 0U), tempctrlQueueSto, Q_DIM(tempctrlQueueSto), (uint8_t *)tempctrl_ao_stack,
                  sizeof(tempctrl_ao_stack), (void *)0);
    
    /* 订阅温度更新事件 */
    QActive_subscribe(&me->super, TEMP_UPDATE_SIG);
    LOG_INF("TempCtrl_ctor: Active Object started and subscribed to TEMP_UPDATE_SIG");
}

/**
 * @brief 温度控制 AO 初始状态
 * 
 * 定义温度控制 Active Object 的初始状态，启动相关的定时器并转换到运行状态。
 * 
 * @param me 指向温度控制 AO 实例的指针
 * @param e 指向事件的指针
 * @return QState 状态转换结果
 */
QState TempCtrl_initial(TempCtrl *const me, QEvt const *const e)
{
    (void)e;
    LOG_INF("TempCtrl_initial: Entering initial state");

    LOG_INF("TempCtrl_initial: Starting display refresh timer");
    /* 启动显示刷新定时器 */
    QTimeEvt_armX(&me->timeEvtDisp, MS2QPC(DISPLAY_REFRESH_INTERVAL_MS), MS2QPC(DISPLAY_REFRESH_INTERVAL_MS));

    LOG_INF("TempCtrl_initial: Starting heat control timer");
    /* 启动加热控制定时器 (每100ms触发一次) */
    QTimeEvt_armX(&me->timeEvtHeat, MS2QPC(HEAT_CONTROL_INTERVAL_MS), MS2QPC(HEAT_CONTROL_INTERVAL_MS));

    LOG_INF("TempCtrl_initial: Transitioning to running state");
    return Q_TRAN(&TempCtrl_running);
}

/**
 * @brief 温度控制 AO 运行状态
 * 
 * 定义温度控制 Active Object 的运行状态，处理各种事件如温度更新、
 * 显示刷新和按钮按下事件。
 * 
 * @param me 指向温度控制 AO 实例的指针
 * @param e 指向事件的指针
 * @return QState 状态转换结果
 */
QState TempCtrl_running(TempCtrl *const me, QEvt const *const e)
{
    QState status;

    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            LOG_INF("Temperature Control AO running");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG:
        {
            status = Q_HANDLED();
            break;
        }

        case TEMP_UPDATE_SIG:
        {
            /* 温度更新事件 - 从温度采样AO接收 */
            TemperatureUpdateEvt *temp_evt = (TemperatureUpdateEvt *)e;
            me->current_temp = temp_evt->temperature;
            me->sample_count = temp_evt->sample_count;

            LOG_DBG("Received temperature update: %.2f", (double)me->current_temp);

            /* 读取温度偏移（来自开关设置） */
            me->temp_offset = switch_read_settings();
            LOG_DBG("temp_offset=%.3f", (double)me->temp_offset);

            /* 更新 PID 控制器 */
            pid_set_setpoint(&me->pid, me->pid_params.setpoint);
            float pid_output = pid_compute(&me->pid, me->current_temp);

            /* 保存PID输出值用于加热控制 */
            me->pid_output = pid_output;

            LOG_INF("PID output=%.3f", (double)pid_output);

            /* 计算显示温度（滤波） */
            me->display_temp = (me->last_display_temp * DISPLAY_LAST_TIMES + me->current_temp + me->pid_params.setpoint * DISPLAY_TARGET_TIMES)
                               / (DISPLAY_TARGET_TIMES + DISPLAY_LAST_TIMES + 1);
            me->last_display_temp = me->display_temp;

            status = Q_HANDLED();
            break;
        }

        case DISPLAY_TIMEOUT_SIG:
        {
            /* 显示刷新定时器超时 */
            display_temp(me->display_temp);
            LOG_DBG("Display updated: %.2f", (double)me->display_temp);
            status = Q_HANDLED();
            break;
        }

        case BUTTON_PRESSED_SIG:
        {
            /* 按键按下 */
            LOG_INF("Button pressed, changing setpoint");

            /* 关闭加热器 */
            gpio_pin_set_dt(&heater, 0);

            /* 循环切换温度档位 */
            me->current_setpoint_idx = (me->current_setpoint_idx + 1) % TEMP_SETPOINT_COUNT;
            me->pid_params.setpoint  = me->setpoints[me->current_setpoint_idx];

            /* 更新 LED 状态 */
            led_set_single(me->current_setpoint_idx);

            /* 重置 PID 控制器 */
            pid_reset(&me->pid);
            pid_set_setpoint(&me->pid, me->pid_params.setpoint);

            LOG_INF("Setpoint changed to %.2f (index=%u)", (double)me->pid_params.setpoint, (unsigned int)me->current_setpoint_idx);

            status = Q_HANDLED();
            break;
        }

        case HEAT_CONTROL_SIG:
        {
            /* 加热控制信号 - 每100ms触发一次 */
            LOG_DBG("Heat control tick, PID output=%.3f", (double)me->pid_output);

            /* 根据PID输出值确定当前周期内的加热次数 (0-10) */
            me->heat_on_count = (uint8_t)(me->pid_output + 0.5f); /* 四舍五入 */
            if (me->heat_on_count > 10)
            {
                me->heat_on_count = 10; /* 限制最大值为10 */
            }

            /* 在10次周期中的第几次开启加热 */
            if (me->heat_cycle_pos < me->heat_on_count)
            {
                /* 开启加热器 */
                gpio_pin_set_dt(&heater, 1);
                LOG_INF("Heater ON (pos=%u, on_count=%u)", (unsigned int)me->heat_cycle_pos, (unsigned int)me->heat_on_count);
            }
            else
            {
                /* 关闭加热器 */
                gpio_pin_set_dt(&heater, 0);
            }

            /* 更新周期位置 */
            me->heat_cycle_pos++;
            if (me->heat_cycle_pos >= 10)
            {
                me->heat_cycle_pos = 0; /* 重置周期 */
            }

            status = Q_HANDLED();
            break;
        }

        default:
        {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}