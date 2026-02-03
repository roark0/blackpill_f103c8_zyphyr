/**
 * @file bsp.c
 * @brief Board Support Package implementation for rt-a19_V2.0 project
 */

#include "bsp.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(bsp, LOG_LEVEL_INF);

/* QPC 定时器 */
static struct k_timer qpc_tick_timer;
static uint32_t tick_count = 0;

/* QPC timer 回调 */
static void qpc_tick_callback(struct k_timer *tid)
{
    Q_UNUSED_PAR(tid);
    tick_count++;

    /* 每 100 个 tick 记录一次 */
    if (tick_count % 100 == 0)
    {
        LOG_DBG("QPC tick #%u", tick_count);
    }

    /* 处理 QPC tick */
    QTimeEvt_tick_(0U, (void *)0);
}

/* QPC Error Handler */
void Q_onError(char const *const module, int const id)
{
    /*
     * NOTE: add here application-specific error handling
     */
    LOG_ERR("Q_onError: module=%s, id=%d", module, id);

    /* Log additional debug information */
    if (strcmp(module, "qf_qact") == 0) {
        LOG_ERR("Q_onError: QF_qact error %d", id);
        if (id == 110) {
            LOG_ERR("Q_onError: QACTIVE_POST_LATE - trying to post before AO is ready");
        }
    }

    /* Trigger breakpoint if running under debugger */
    k_panic();

    /* NOTE: Q_onError() MUST NOT return to avoid undefined behavior */
    for (;;)
    {
        /* Hang forever */
    }
}

/* QPC Tick Handler */
void QF_onClockTick(void)
{
    /* QF_TICK_X(0U) must be called in the context of a thread */
    QTimeEvt_tick_(0U, (void *)0);
}

/* QPC Startup Handler */
void QF_onStartup(void)
{
    LOG_INF("QPC framework started");

    /* 启动 QPC tick 定时器（10ms 间隔） */
    k_timer_init(&qpc_tick_timer, qpc_tick_callback, NULL);
    k_timer_start(&qpc_tick_timer, K_MSEC(10), K_MSEC(10));
    LOG_INF("QPC tick timer started (10ms interval)");
}