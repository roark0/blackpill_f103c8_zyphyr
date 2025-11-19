#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define SLEEP_TIME_MS 1000

int main(void)
{
    int ret;
    bool led_state = true;

    printk("Starting LED blinky application\n");

    if (!device_is_ready(led.port))
    {
        printk("GPIO device not ready\n");
        return 0;
    }

    printk("GPIO device is ready\n");
    printk("GPIO port: %p, pin: %d, flags: %d\n", led.port, led.pin, led.dt_flags);

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        printk("Failed to configure GPIO pin (err %d)\n", ret);
        return 0;
    }

    printk("GPIO pin configured successfully\n");

    // 个闪烁LED的循环
    while (1)
    {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0)
        {
            printk("Failed to toggle GPIO pin (err %d)\n", ret);
            return 0;
        }

        printk("LED toggled, state: %s\n", led_state ? "ON" : "OFF");
        led_state = !led_state;
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
