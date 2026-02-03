/**
 * @file temperature_sampling.h
 * @brief Temperature Sampling Active Object
 * 
 * This file defines the interface for the Temperature Sampling Active Object,
 * which handles temperature reading from DS18B20 sensors and publishes
 * temperature updates to other Active Objects.
 */

#ifndef TEMPERATURE_SAMPLING_H
#define TEMPERATURE_SAMPLING_H

#include "qpc.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C"
{
#endif

//!< 温度采样工作队列栈大小
#define TEMP_SAMPLING_WORK_STACK_SIZE 1024
//!< 温度采样工作队列优先级
#define TEMP_SAMPLING_WORK_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

//!< Active Object 栈大小
#define TEMP_SAMPLING_AO_STACK_SIZE 2048
//!< Active Object 优先级
#define TEMP_SAMPLING_AO_PRIORITY 7

/**
 * @brief 温度更新事件结构体
 * 
 * 定义了温度更新事件的数据结构，包含温度值和采样计数。
 */
typedef struct {
    QEvt super;          //!< QPC 事件基类
    float temperature;   //!< 温度值
    uint32_t sample_count;  //!< 采样计数
} TemperatureUpdateEvt;

/**
 * @brief 温度采样 Active Object 结构体
 * 
 * 定义了温度采样 Active Object 的数据结构，包含定时器、
 * 工作队列和温度数据等。
 */
typedef struct {
    QActive super;         //!< QPC Active Object 基类

    /* 定时器 */
    QTimeEvt timeEvtTemp;  //!< 温度采样定时器

    /* 温度采样工作队列（用于异步执行 DS18B20 读取） */
    struct k_work temp_work;    //!< 工作项
    struct k_work_q temp_work_q; //!< 工作队列

    /* 温度数据 */
    float current_temp;     //!< 当前温度值
    uint32_t sample_count;  //!< 采样计数

} TempSampler;

/**
 * @brief 构造温度采样 Active Object
 * 
 * 初始化温度采样 Active Object，设置初始参数和启动 AO。
 */
void TempSampler_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_SAMPLING_H */