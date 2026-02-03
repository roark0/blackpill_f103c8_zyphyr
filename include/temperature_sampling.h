/**
 * @file temperature_sampling.h
 * @brief Temperature Sampling Active Object
 */

#ifndef TEMPERATURE_SAMPLING_H
#define TEMPERATURE_SAMPLING_H

#include "qpc.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* 温度采样工作队列配置 */
#define TEMP_SAMPLING_WORK_STACK_SIZE 1024
#define TEMP_SAMPLING_WORK_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

/* Active Object 配置 */
#define TEMP_SAMPLING_AO_STACK_SIZE 2048
#define TEMP_SAMPLING_AO_PRIORITY 7

/* 温度更新事件 */
typedef struct {
    QEvt super;
    float temperature;
    uint32_t sample_count;
} TemperatureUpdateEvt;

/* 温度采样 Active Object */
typedef struct {
    QActive super;

    /* 定时器 */
    QTimeEvt timeEvtTemp;   /* 温度采样定时器 */

    /* 温度采样工作队列（用于异步执行 DS18B20 读取） */
    struct k_work temp_work;
    struct k_work_q temp_work_q;

    /* 温度数据 */
    float current_temp;
    uint32_t sample_count;

} TempSampler;

/**
 * @brief 构造温度采样 Active Object
 */
void TempSampler_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_SAMPLING_H */