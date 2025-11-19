#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* 定义UART设备 */
// #define USART_DEVICE_NAME DT_LABEL(DT_NODELABEL(usart1))
// #define UART_DEVICE_NODE DT_NODELABEL(usart1)

/* 定义GPIO引脚PA11用于blinky */

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define SLEEP_TIME_MS 1000


int main(void)
{
    int ret;
    bool led_state = true;

    if (!gpio_is_ready_dt(&led))
    {
        return 0;
    }
    // gpio_pin_set(&led, 0, led_state);

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        return 0;
    }

#if 0
    printk("UART1 and GPIO PA11 initialized successfully\n");

    // 发送一些调试信息通过UART
    uart_poll_out(uart_dev, 'H');
    uart_poll_out(uart_dev, 'e');
    uart_poll_out(uart_dev, 'l');
    uart_poll_out(uart_dev, 'l');
    uart_poll_out(uart_dev, 'o');
    uart_poll_out(uart_dev, ' ');
    uart_poll_out(uart_dev, 'U');
    uart_poll_out(uart_dev, 'A');
    uart_poll_out(uart_dev, 'R');
    uart_poll_out(uart_dev, 'T');
    uart_poll_out(uart_dev, '1');
    uart_poll_out(uart_dev, ' ');
    uart_poll_out(uart_dev, 'a');
    uart_poll_out(uart_dev, 'n');
    uart_poll_out(uart_dev, 'd');
    uart_poll_out(uart_dev, ' ');
    uart_poll_out(uart_dev, 'G');
    uart_poll_out(uart_dev, 'P');
    uart_poll_out(uart_dev, 'I');
    uart_poll_out(uart_dev, 'O');
    uart_poll_out(uart_dev, ' ');
    uart_poll_out(uart_dev, 'P');
    uart_poll_out(uart_dev, 'A');
    uart_poll_out(uart_dev, '1');
    uart_poll_out(uart_dev, '1');
    uart_poll_out(uart_dev, '\r');
    uart_poll_out(uart_dev, '\n');
#endif

    // 闪烁LED的循环 - 使用PA11引脚

    while (1)
    {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0)
        {
            return 0;
        }

        led_state = !led_state;
        // printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
