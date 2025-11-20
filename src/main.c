#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "gpio_control.h"
#include "keyboard_logic.h"
#include "uart_wrapper.h"

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define SLEEP_TIME_MS 50

int main(void)
{
    int ret;
    printk("Starting dual LED blinky application\n");

    // 初始化GPIO控制模块（键盘控制）
    GPIO_Control_Init();

    // 检查GPIO设备是否就绪
    if (!device_is_ready(led0.port))
    {
        printk("GPIO device for LED0 not ready\n");
        return 0;
    }

    if (!device_is_ready(led1.port))
    {
        printk("GPIO device for LED1 not ready\n");
        return 0;
    }

    // 配置GPIO引脚
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        printk("Failed to configure GPIO pin for LED0 (err %d)\n", ret);
        return 0;
    }

    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        printk("Failed to configure GPIO pin for LED1 (err %d)\n", ret);
        return 0;
    }

    printk("GPIO pins configured successfully\n");

    // LED闪烁控制计数器（用于实现1Hz闪烁）
    int led_toggle_counter = 0;
    const int led_toggle_interval = 10; // 10 * 50ms = 500ms，即0.5秒，实现1Hz闪烁

    while (1)
    {
        // 每500ms切换一次LED状态，实现1Hz闪烁
        if (led_toggle_counter >= led_toggle_interval)
        {
            ret = gpio_pin_toggle_dt(&led0);
            if (ret < 0)
            {
                printk("Failed to toggle GPIO pin for LED0 (err %d)\n", ret);
                return 0;
            }

            ret = gpio_pin_toggle_dt(&led1);
            if (ret < 0)
            {
                printk("Failed to toggle GPIO pin for LED1 (err %d)\n", ret);
                return 0;
            }
            
            led_toggle_counter = 0; // 重置计数器
        }
        else
        {
            led_toggle_counter++; // 增加计数器
        }
        
        Keyboard_Scan_And_Transmit();

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
