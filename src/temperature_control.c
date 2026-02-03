/**
 * @file temperature_control.c
 * @brief Temperature Control Active Object implementation
 */

#include "temperature_control.h"
#include "ds18b20.h"
#include "led.h"
#include "display.h"
#include "switch.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tempctrl, LOG_LEVEL_INF);

/* GPIO 设备节点 */
#define HEATER_NODE DT_ALIAS(heater)
static const struct gpio_dt_spec heater = GPIO_DT_SPEC_GET(HEATER_NODE, gpios);

/* 温度控制 AO 实例 */
static TempCtrl l_tempCtrl;

/* AO 状态转换声明 */
QState TempCtrl_initial(TempCtrl * const me, QEvt const * const e);
QState TempCtrl_running(TempCtrl * const me, QEvt const * const e);

/* 定时器间隔 */
#define TEMP_SAMPLE_INTERVAL_MS  1000    /* 温度采样间隔 1秒 */
#define DISPLAY_REFRESH_INTERVAL_MS 300  /* 显示刷新间隔 300ms */

/* 显示滤波参数 */
#define DISPLAY_TARGET_TIMES 0
#define DISPLAY_LAST_TIMES 10

/* 加热器输出阈值 */
#define HEATER_OUTPUT_THRESHOLD 0.15f

/* 温度控制 AO 构造函数 */
void TempCtrl_ctor(void) {
    TempCtrl *me = &l_tempCtrl;
    QActive_ctor(&me->super, Q_STATE_CAST(&TempCtrl_initial));

    /* 初始化定时器 */
    QTimeEvt_ctorX(&me->timeEvtTemp, &me->super, TEMP_TIMEOUT_SIG, 0U);
    QTimeEvt_ctorX(&me->timeEvtDisp, &me->super, DISPLAY_TIMEOUT_SIG, 0U);

    /* 初始化温度数据 */
    me->current_temp = 0.0f;
    me->display_temp = 0.0f;
    me->last_display_temp = 37.0f;
    me->sample_count = 0;
    me->current_setpoint_idx = 2;
    me->temp_offset = 0.0f;

    /* 初始化温度设定值 */
    me->setpoints[0] = 25.0f;
    me->setpoints[1] = 30.0f;
    me->setpoints[2] = 37.0f;
    me->setpoints[3] = 45.0f;

    /* 初始化 PID 控制器参数 */
    me->pid_params.kp = 1.0f;
    me->pid_params.ki = 0.05f;
    me->pid_params.kd = 0.3f;
    me->pid_params.setpoint = me->setpoints[2];
    me->pid_params.output_min = 0.0f;
    me->pid_params.output_max = 1.0f;

    /* 初始化 PID 控制器 */
    pid_init(&me->pid,
             me->pid_params.kp,
             me->pid_params.ki,
             me->pid_params.kd,
             me->pid_params.setpoint,
             me->pid_params.output_min,
             me->pid_params.output_max);
}

/* 初始状态 */
QState TempCtrl_initial(TempCtrl * const me, QEvt const * const e) {
    (void)e;

    LOG_INF("Temperature Control AO initialized");

    /* 启动温度采样定时器 */
    QTimeEvt_armX(&me->timeEvtTemp, MS2QPC(TEMP_SAMPLE_INTERVAL_MS),
                  MS2QPC(TEMP_SAMPLE_INTERVAL_MS));

    /* 启动显示刷新定时器 */
    QTimeEvt_armX(&me->timeEvtDisp, MS2QPC(DISPLAY_REFRESH_INTERVAL_MS),
                  MS2QPC(DISPLAY_REFRESH_INTERVAL_MS));

    return Q_TRAN(&TempCtrl_running);
}

/* 运行状态 */
QState TempCtrl_running(TempCtrl * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
    case Q_ENTRY_SIG: {
        LOG_INF("Temperature Control AO running");
        status = Q_HANDLED();
        break;
    }

    case Q_EXIT_SIG: {
        status = Q_HANDLED();
        break;
    }

    case TEMP_TIMEOUT_SIG: {
        /* 温度采样定时器超时 */
        me->sample_count++;

        /* 读取温度偏移（来自开关设置） */
        me->temp_offset = switch_read_settings();
        LOG_DBG("temp_offset=%.3f", (double)me->temp_offset);

        /* 读取 DS18B20 温度 */
        me->current_temp = ds18b20_read_temperature(me->temp_offset);
        LOG_INF("Sample #%u: temp=%.2f°C, setpoint=%.2f°C",
                (unsigned int)me->sample_count,
                (double)me->current_temp,
                (double)me->pid_params.setpoint);

        /* 更新 PID 控制器 */
        pid_set_setpoint(&me->pid, me->pid_params.setpoint);
        float pid_output = pid_compute(&me->pid, me->current_temp);

        /* PID 输出控制加热器 */
        if (pid_output > HEATER_OUTPUT_THRESHOLD && me->current_temp > 0.0f) {
            gpio_pin_set_dt(&heater, 1);
            LOG_DBG("Heater ON, pid_output=%.3f", (double)pid_output);
        } else {
            gpio_pin_set_dt(&heater, 0);
        }

        /* 计算显示温度（滤波） */
        me->display_temp = (me->last_display_temp * DISPLAY_LAST_TIMES +
                            me->current_temp +
                            me->pid_params.setpoint * DISPLAY_TARGET_TIMES) /
                           (DISPLAY_TARGET_TIMES + DISPLAY_LAST_TIMES + 1);
        me->last_display_temp = me->display_temp;

        status = Q_HANDLED();
        break;
    }

    case DISPLAY_TIMEOUT_SIG: {
        /* 显示刷新定时器超时 */
        display_temp(me->display_temp);
        LOG_DBG("Display updated: %.2f°C", (double)me->display_temp);
        status = Q_HANDLED();
        break;
    }

    case BUTTON_PRESSED_SIG: {
        /* 按键按下 */
        LOG_INF("Button pressed, changing setpoint");

        /* 关闭加热器 */
        gpio_pin_set_dt(&heater, 0);

        /* 循环切换温度档位 */
        me->current_setpoint_idx = (me->current_setpoint_idx + 1) % TEMP_SETPOINT_COUNT;
        me->pid_params.setpoint = me->setpoints[me->current_setpoint_idx];

        /* 更新 LED 状态 */
        led_set_single(me->current_setpoint_idx);

        /* 重置 PID 控制器 */
        pid_reset(&me->pid);
        pid_set_setpoint(&me->pid, me->pid_params.setpoint);

        LOG_INF("Setpoint changed to %.2f°C (index=%u)",
                (double)me->pid_params.setpoint,
                (unsigned int)me->current_setpoint_idx);

        status = Q_HANDLED();
        break;
    }

    case SETPOINT_CHANGE_SIG: {
        /* 设定值改变事件 */
        SetpointChangeEvent *evt = (SetpointChangeEvent *)e;
        if (evt->setpoint_idx < TEMP_SETPOINT_COUNT) {
            me->current_setpoint_idx = evt->setpoint_idx;
            me->pid_params.setpoint = me->setpoints[me->current_setpoint_idx];

            led_set_single(me->current_setpoint_idx);
            pid_reset(&me->pid);
            pid_set_setpoint(&me->pid, me->pid_params.setpoint);

            LOG_INF("Setpoint changed to %.2f°C (index=%u)",
                    (double)me->pid_params.setpoint,
                    (unsigned int)me->current_setpoint_idx);
        }
        status = Q_HANDLED();
        break;
    }

    default: {
        status = Q_SUPER(&QHsm_top);
        break;
    }
    }

    return status;
}