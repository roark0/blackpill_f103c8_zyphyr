#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "gpio_control.h"
#include "keyboard_logic.h"
#include "uart_wrapper.h"

#define SLEEP_TIME_MS 50

int main(void)
{
    printk("Starting RT-9000 Keyboard application V1.0\n");

    // 初始化GPIO控制模块（键盘控制）
    GPIO_Control_Init();

    // LED闪烁控制计数器（用于实现1Hz闪烁）
    int led_toggle_counter        = 0;
    const int led_toggle_interval = 10;  // 10 * 50ms = 500ms，即0.5秒，实现1Hz闪烁

    while (1)
    {
        // 每500ms切换一次LED状态，实现1Hz闪烁
        if (led_toggle_counter >= led_toggle_interval)
        {
            GPIO_ToggleLED(0);
            GPIO_ToggleLED(1);

            led_toggle_counter = 0;  // 重置计数器
        }
        else
        {
            led_toggle_counter++;  // 增加计数器
        }

        Keyboard_Scan_And_Transmit();

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
