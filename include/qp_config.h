/**
 * @file qp_config.h
 * @brief QPC Framework Configuration for rt-a19_V2.0 project
 */

#ifndef QP_CONFIG_H
#define QP_CONFIG_H

/* QPC Framework Configuration - 参考官方 blinky 示例 */
#define QF_MAX_ACTIVE CONFIG_NUM_PREEMPT_PRIORITIES  /* Maximum # Active Objects */
#define QF_MAX_EPOOL 3U                                 /* Maximum # Event Pools */
#define QF_MAX_TICK_RATE 1U                             /* Maximum # clock tick rates */
#define QF_EVENT_SIZ_SIZE 2U                            /* Size of dynamic events */
#define QF_TIMEEVT_CTR_SIZE 4U                          /* Size of QTimeEvt counter */
#define QF_EQUEUE_CTR_SIZE 1U                           /* Size of event queue counter */
#define QF_MPOOL_CTR_SIZE 2U                            /* Size of memory pool counter */
#define QF_MPOOL_SIZ_SIZE 2U                            /* Size of memory pool block */

/* Zephyr 特定配置 */
#define QF_ON_CONTEXT_SW 1           /* 启用上下文切换支持 */

/* 禁用 QSPY tracing (开源版本不支持) */
#undef Q_SPY

#endif /* QP_CONFIG_H */