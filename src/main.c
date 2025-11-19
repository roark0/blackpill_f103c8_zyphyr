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

#define SLEEP_TIME_MS 100

int main(void)
{
    int ret;
    bool led0_state = true;
    bool led1_state = false;

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

    printk("GPIO devices are ready\n");
    printk("LED0 - GPIO port: %p, pin: %d, flags: %d\n", led0.port, led0.pin, led0.dt_flags);
    printk("LED1 - GPIO port: %p, pin: %d, flags: %d\n", led1.port, led1.pin, led1.dt_flags);

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


    // 闪烁两个LED的循环，并每500ms通过UART发送"hello world"
    while (1)
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

        led0_state = !led0_state;
        led1_state = !led1_state;

        // printk("LED0 toggled, state: %s | LED1 toggled, state: %s\n", led0_state ? "ON1" : "OFF", led1_state ? "ON" : "OFF");

        // print_uart("hello world\r\n");
        Keyboard_Scan_And_Transmit();
        GPIO_SetKeyRows(0);

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
