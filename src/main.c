#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/arch/cpu.h>
#include "ds18b20.h"
#include "led.h"
#include "switch.h"
#include "display.h"

#include "qp.h"

/* Temperature Control Active Object */
#include "temperature_control.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// GPIO 设备节点
#define HEATER_NODE DT_ALIAS(heater)
static const struct gpio_dt_spec heater = GPIO_DT_SPEC_GET(HEATER_NODE, gpios);

// 主函数
int main(void)
{
    int ret;

    /* 初始化 QPC 框架 */
    QF_init();
    LOG_INF("QPC framework initialized");

    /* 初始化事件池 */
    static QF_MPOOL_EL(QEvt) eventPoolSto[32];
    QF_poolInit(eventPoolSto, sizeof(eventPoolSto), sizeof(QEvt));
    LOG_INF("QPC event pool initialized (32 events)");

    /* 初始化发布订阅 */
    static QSubscrList subscrSto[16];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));
    LOG_INF("QPC publish-subscribe initialized");

    LOG_INF("Temperature Control System Starting...");

    // 初始化 LED
    ret = led_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize LEDs");
        return ret;
    }

    // 初始化按键中断
    ret = button_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize button interrupt");
        return ret;
    }

    // 初始化开关
    ret = switch_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize switches");
        return ret;
    }

    // 初始化 DS18B20
    if (ds18b20_init() != 0)
    {
        LOG_ERR("Failed to initialize DS18B20");
        // return -1;
    }

    // 初始化加热器 GPIO
    if (!device_is_ready(heater.port))
    {
        LOG_ERR("Heater device not ready");
        return -1;
    }

    // 配置加热器 GPIO
    ret = gpio_pin_configure_dt(&heater, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure heater");
        return ret;
    }

    // 初始化 LED 状态
    led_set_single(2);  // 默认点亮第 3 个 LED（索引 2）
    gpio_pin_set_dt(&heater, 0);

    // 初始化数码管
    LOG_INF("Initializing display...");
    ret = display_init();
    if (ret != 0)
    {
        LOG_ERR("Failed to initialize display");
        return ret;
    }
    LOG_INF("Display initialized successfully");

    /* 初始化温度控制 Active Object */
    LOG_INF("Initializing Temperature Control AO...");
    TempCtrl_ctor();

    /* 启动 QPC 事件循环 */
    LOG_INF("Starting QPC event loop");
    return QF_run();
}