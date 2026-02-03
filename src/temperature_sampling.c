/**
 * @file temperature_sampling.c
 * @brief Temperature Sampling Active Object implementation
 * 
 * This file implements the Temperature Sampling Active Object, which handles
 * temperature reading from DS18B20 sensors using a work queue for asynchronous
 * execution and publishes temperature updates to other Active Objects.
 */

#include "temperature_sampling.h"
#include "ds18b20.h"
#include "switch.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tempsampler, LOG_LEVEL_INF);

//!< 温度采样 AO 实例
TempSampler l_tempSampler;

//!< 温度采样工作队列栈
static K_THREAD_STACK_DEFINE(temp_sampling_work_stack, TEMP_SAMPLING_WORK_STACK_SIZE);

//!< Active Object 线程栈
K_THREAD_STACK_DEFINE(temp_sampling_ao_stack, TEMP_SAMPLING_AO_STACK_SIZE);

//!< Active Object 事件队列（50个事件）
static QEvtPtr temp_sampling_queue_sto[50];

/* AO 状态转换声明 */
QState TempSampler_initial(TempSampler *const me, QEvt const *const e);
QState TempSampler_running(TempSampler *const me, QEvt const *const e);

/* 前向声明 */
static void temp_sampling_work_handler(struct k_work *work);

//!< 温度采样间隔（毫秒）
#define TEMP_SAMPLE_INTERVAL_MS 1000     

/**
 * @brief 温度采样工作函数（在工作队列中异步执行）
 * 
 * 该函数在工作队列线程中异步执行温度读取操作，避免阻塞 Active Object。
 * 读取温度值后，发布温度更新事件给订阅者。
 * 
 * @param work 指向工作项的指针
 */
static void temp_sampling_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    TempSampler *me = &l_tempSampler;

    LOG_DBG("Temperature sampling work handler started");

    /* 读取温度偏移（来自开关设置） */
    float temp_offset = switch_read_settings();
    LOG_DBG("temp_offset=%.3f", (double)temp_offset);

    /* 异步读取 DS18B20 温度（耗时操作） */
    int64_t start    = k_uptime_get();
    float raw_temp   = ds18b20_read_temperature();  // 读取原始温度
    float adjusted_temp = raw_temp + temp_offset;  // 应用偏移
    int64_t elapsed  = k_uptime_get() - start;

    me->sample_count++;

    if (raw_temp < -100.0f)
    {
        /* 温度读取失败 */
        LOG_ERR("Temperature read failed after %lld ms", elapsed);
    }
    else
    {
        LOG_INF("Sample #%u: raw_temp=%.2f, adjusted_temp=%.2f", (unsigned int)me->sample_count, (double)raw_temp, (double)adjusted_temp);
        me->current_temp = adjusted_temp;

        /* 发布温度更新事件 */
        TemperatureUpdateEvt *temp_evt = Q_NEW(TemperatureUpdateEvt, TEMP_UPDATE_SIG);
        if (temp_evt != NULL)
        {
            temp_evt->temperature = adjusted_temp;
            temp_evt->sample_count = me->sample_count;
            QACTIVE_PUBLISH((QEvt *)temp_evt, &l_tempSampler.super);
        }
    }
}

/**
 * @brief 温度采样 AO 构造函数
 * 
 * 初始化温度采样 Active Object，设置初始参数和启动 AO。
 * 该函数会初始化工作队列、定时器并启动 Active Object。
 */
void TempSampler_ctor(void)
{
    TempSampler *me = &l_tempSampler;
    LOG_INF("TempSampler_ctor: Starting");

    /* 初始化温度采样工作队列 */
    LOG_DBG("TempSampler_ctor: Initializing work queue");
    k_work_queue_init(&me->temp_work_q);
    k_work_queue_start(&me->temp_work_q, temp_sampling_work_stack, 
                       K_THREAD_STACK_SIZEOF(temp_sampling_work_stack),
                       TEMP_SAMPLING_WORK_PRIORITY, NULL);
    LOG_DBG("TempSampler_ctor: Work queue started");

    k_work_init(&me->temp_work, temp_sampling_work_handler);
    LOG_DBG("TempSampler_ctor: Work item initialized");

    LOG_DBG("TempSampler_ctor: Constructing QActive");
    QActive_ctor(&me->super, Q_STATE_CAST(&TempSampler_initial));

    /* 构造时间事件 */
    QTimeEvt_ctorX(&me->timeEvtTemp, &me->super, TEMP_TIMEOUT_SIG, 0U);

    /* 初始化温度数据 */
    me->current_temp = 0.0f;
    me->sample_count = 0;

    /* 启动活动对象 */
    LOG_DBG("TempSampler_ctor: Starting Active Object");
    QActive_start(&me->super,
                  Q_PRIO(TEMP_SAMPLING_AO_PRIORITY, 0U),
                  temp_sampling_queue_sto,
                  Q_DIM(temp_sampling_queue_sto),
                  (uint8_t *)temp_sampling_ao_stack,
                  sizeof(temp_sampling_ao_stack),
                  (void *)0);
    LOG_INF("TempSampler_ctor: Active Object started");
}

/**
 * @brief 温度采样 AO 初始状态
 * 
 * 定义温度采样 Active Object 的初始状态，启动相关的定时器并转换到运行状态。
 * 
 * @param me 指向温度采样 AO 实例的指针
 * @param e 指向事件的指针
 * @return QState 状态转换结果
 */
QState TempSampler_initial(TempSampler *const me, QEvt const *const e)
{
    (void)e;
    LOG_INF("TempSampler_initial: Entering initial state");

    LOG_INF("TempSampler_initial: Starting temperature sampling timer");
    /* 启动温度采样定时器 */
    QTimeEvt_armX(&me->timeEvtTemp, MS2QPC(TEMP_SAMPLE_INTERVAL_MS),
                  MS2QPC(TEMP_SAMPLE_INTERVAL_MS));

    LOG_INF("TempSampler_initial: Transitioning to running state");
    return Q_TRAN(&TempSampler_running);
}

/**
 * @brief 温度采样 AO 运行状态
 * 
 * 定义温度采样 Active Object 的运行状态，处理温度采样定时器超时事件，
 * 提交工作到工作队列异步执行温度读取。
 * 
 * @param me 指向温度采样 AO 实例的指针
 * @param e 指向事件的指针
 * @return QState 状态转换结果
 */
QState TempSampler_running(TempSampler *const me, QEvt const *const e)
{
    QState status;

    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            LOG_INF("Temperature Sampling AO running");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG:
        {
            status = Q_HANDLED();
            break;
        }

        case TEMP_TIMEOUT_SIG:
        {
            /* 温度采样定时器超时 - 提交工作到工作队列异步执行 */
            LOG_DBG("Temperature timeout, submitting work to queue");
            k_work_submit_to_queue(&l_tempSampler.temp_work_q, &l_tempSampler.temp_work);
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