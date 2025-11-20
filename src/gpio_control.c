#include "gpio_control.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

// #include <stm32f1xx_hal.h>

/* 定义键盘行和列的GPIO引脚 (直接使用GPIO端口和引脚号) */
static const struct device *kb_port = DEVICE_DT_GET(DT_NODELABEL(gpiob));  // GPIOB for all keyboard pins

/* LED 引脚定义 */
static struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);
static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_NODELABEL(led2), gpios);

/**
 * @brief 初始化GPIO控制模块
 * @retval None
 */
void GPIO_Control_Init(void)
{
    int ret;

    // 对于STM32F103，需要禁用JTAG/SWD功能以允许PB4用作普通GPIO
    // 通过AFIO的SWJ_CFG位来禁用JTAG功能
    // __HAL_RCC_AFIO_CLK_ENABLE();
    // __HAL_AFIO_REMAP_SWJ_NOJTAG();  // 只禁用JTAG，保留SWD

    // 检查GPIO设备是否就绪
    if (!device_is_ready(kb_port))
    {
        printk("Keyboard GPIOB device not ready\n");
        return;
    }

    // 初始化键盘行控制引脚 (输出) - PB4-PB7 (KSCON0-KSCON3)
    ret = gpio_pin_configure(kb_port, 4, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure KSCON0 (PB4) GPIO pin (err %d)\n", ret);
        return;
    }

    ret = gpio_pin_configure(kb_port, 5, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure KSCON1 (PB5) GPIO pin (err %d)\n", ret);
        return;
    }

    ret = gpio_pin_configure(kb_port, 6, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure KSCON2 (PB6) GPIO pin (err %d)\n", ret);
        return;
    }

    ret = gpio_pin_configure(kb_port, 7, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure KSCON3 (PB7) GPIO pin (err %d)\n", ret);
        return;
    }

    // 初始化键盘列读取引脚 (输入) - PB0-PB3 (KSNS0-KSNS3)
    ret = gpio_pin_configure(kb_port, 0, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure KSNS0 (PB0) GPIO pin (err %d)\n", ret);
        return;
    }

    ret = gpio_pin_configure(kb_port, 1, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure KSNS1 (PB1) GPIO pin (err %d)\n", ret);
        return;
    }

    ret = gpio_pin_configure(kb_port, 2, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure KSNS2 (PB2) GPIO pin (err %d)\n", ret);
        return;
    }

    ret = gpio_pin_configure(kb_port, 3, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure KSNS3 (PB3) GPIO pin (err %d)\n", ret);
        return;
    }

    // 初始化LED引脚
    if (!device_is_ready(led1.port))
    {
        printk("LED1 GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        printk("Failed to configure LED1 GPIO pin (err %d)\n", ret);
        return;
    }

    if (!device_is_ready(led2.port))
    {
        printk("LED2 GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        printk("Failed to configure LED2 GPIO pin (err %d)\n", ret);
        return;
    }

    printk("GPIO control module initialized successfully\n");
}

/**
 * @brief 设置键盘控制列的状态
 * @param row_pattern: 行模式 (位0-3对应KSCON0-KSCON3)
 * @retval None
 */
void GPIO_SetKeyRows(uint8_t row_pattern)
{
    gpio_pin_set(kb_port, 4, (row_pattern & 0x01) ? 1 : 0);  // KSCON0 - PB4
    gpio_pin_set(kb_port, 5, (row_pattern & 0x02) ? 1 : 0);  // KSCON1 - PB5
    gpio_pin_set(kb_port, 6, (row_pattern & 0x04) ? 1 : 0);  // KSCON2 - PB6
    gpio_pin_set(kb_port, 7, (row_pattern & 0x08) ? 1 : 0);  // KSCON3 - PB7
}

/**
 * @brief 设置行输出
 * @param value: 输出值
 * @retval None
 */
void Set_Row_Output(uint8_t value)
{
    GPIO_SetKeyRows(value);
}

/**
 * @brief 设置单个键盘控制列的状态
 * @param row: 列号 (0-3对应KSCON0-KSCON3)
 * @param state: 状态 (0=低电平, 1=高电平)
 * @retval None
 */
void GPIO_SetKeyRow(uint8_t row, uint8_t state)
{
    switch (row)
    {
        case 0:
            gpio_pin_set(kb_port, 4, state ? 1 : 0);  // KSCON0 - PB4
            break;
        case 1:
            gpio_pin_set(kb_port, 5, state ? 1 : 0);  // KSCON1 - PB5
            break;
        case 2:
            gpio_pin_set(kb_port, 6, state ? 1 : 0);  // KSCON2 - PB6
            break;
        case 3:
            gpio_pin_set(kb_port, 7, state ? 1 : 0);  // KSCON3 - PB7
            break;
        default:
            break;
    }
}

/**
 * @brief 读取所有键盘扫描行的状态
 * @retval uint8_t: 行状态 (位0-3对应KSNS0-KSNS3)
 */
uint8_t GPIO_ReadKeyCols(void)
{
    uint8_t col_state = 0;

    if (gpio_pin_get(kb_port, 0) == 0)  // KSNS0 - PB0, 低电平表示按键按下
        col_state |= 0x01;
    if (gpio_pin_get(kb_port, 1) == 0)  // KSNS1 - PB1
        col_state |= 0x02;
    if (gpio_pin_get(kb_port, 2) == 0)  // KSNS2 - PB2
        col_state |= 0x04;
    if (gpio_pin_get(kb_port, 3) == 0)  // KSNS3 - PB3
        col_state |= 0x08;

    return col_state;
}

/**
 * @brief 读取列输入
 * @retval uint8_t: 列状态
 */
uint8_t Read_Column_Input(void)
{
    return GPIO_ReadKeyCols();
}

/**
 * @brief 读取单个键盘扫描行的状态
 * @param col: 行号 (0-3对应KSNS0-KSNS3)
 * @retval uint8_t: 行状态 (0=高电平, 1=低电平)
 */
uint8_t GPIO_ReadKeyCol(uint8_t col)
{
    int pin_state = 1;  // 默认为高电平

    switch (col)
    {
        case 0:
            pin_state = gpio_pin_get(kb_port, 0);  // KSNS0 - PB0
            break;
        case 1:
            pin_state = gpio_pin_get(kb_port, 1);  // KSNS1 - PB1
            break;
        case 2:
            pin_state = gpio_pin_get(kb_port, 2);  // KSNS2 - PB2
            break;
        case 3:
            pin_state = gpio_pin_get(kb_port, 3);  // KSNS3 - PB3
            break;
        default:
            break;
    }

    return (pin_state == 0) ? 1 : 0;  // 0表示低电平(按键按下)，1表示高电平(按键释放)
}

/**
 * @brief 设置LED状态
 * @param led_num: LED编号 (1或2)
 * @param state: 状态 (0=熄灭, 1=点亮)
 * @retval None
 */
void GPIO_SetLED(uint8_t led_num, uint8_t state)
{
    switch (led_num)
    {
        case 1:
            gpio_pin_set_dt(&led1, state ? 1 : 0);
            break;
        case 2:
            gpio_pin_set_dt(&led2, state ? 1 : 0);
            break;
        default:
            break;
    }
}

/**
 * @brief 切换LED状态
 * @param led_num: LED编号 (1或2)
 * @retval None
 */
void GPIO_ToggleLED(uint8_t led_num)
{
    int ret;
    switch (led_num)
    {
        case 1:
            ret = gpio_pin_toggle_dt(&led1);
            if (ret < 0)
            {
                printk("Failed to toggle LED1 (err %d)\n", ret);
            }
            break;
        case 2:
            ret = gpio_pin_toggle_dt(&led2);
            if (ret < 0)
            {
                printk("Failed to toggle LED2 (err %d)\n", ret);
            }
            break;
        default:
            break;
    }
}

/**
 * @brief 扫描键盘状态
 * @retval uint16_t: 键盘扫描码
 */
uint16_t GPIO_ScanKeyboard(void)
{
    uint16_t key_code = 0;

    // 扫描每一列 (KSCON0-KSCON3)
    for (uint8_t col = 0; col < 4; col++)
    {
        // 设置当前列为低电平，其余为高电平
        GPIO_SetKeyRows(~(1 << col));

        // 短暂延时以稳定信号
        k_msleep(1);

        // 读取行状态 (KSNS0-KSNS3)
        uint8_t row_state = GPIO_ReadKeyCols();

        // 如果有按键按下，记录列和行
        if (row_state != 0x0F)
        {  // 0x0F表示所有行都是高电平(无按键)
            for (uint8_t row = 0; row < 4; row++)
            {
                if ((row_state & (1 << row)) == 0)
                {  // 检测到低电平(按键按下)
                    // 计算键码 (列*4 + 行)
                    key_code = (col << 8) | row;
                    break;
                }
            }
        }

        // 如果已经检测到按键，退出循环
        if (key_code != 0)
        {
            break;
        }
    }

    // 恢复所有列为高电平
    GPIO_SetKeyRows(0xFF);

    return key_code;
}