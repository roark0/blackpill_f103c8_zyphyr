#include "gpio_control.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include <stm32f1xx_hal.h>

/* 定义键盘行和列的GPIO引脚 (使用设备树定义的节点) */
static struct gpio_dt_spec kb_rows[6] = {GPIO_DT_SPEC_GET(DT_NODELABEL(row1), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(row2), gpios),
                                         GPIO_DT_SPEC_GET(DT_NODELABEL(row3), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(row4), gpios),
                                         GPIO_DT_SPEC_GET(DT_NODELABEL(row5), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(row6), gpios)};

static struct gpio_dt_spec kb_cols[8] = {GPIO_DT_SPEC_GET(DT_NODELABEL(col1), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(col2), gpios),
                                         GPIO_DT_SPEC_GET(DT_NODELABEL(col3), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(col4), gpios),
                                         GPIO_DT_SPEC_GET(DT_NODELABEL(col5), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(col6), gpios),
                                         GPIO_DT_SPEC_GET(DT_NODELABEL(col7), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(col8), gpios)};

/* 定义特殊按键GPIO引脚 (使用设备树定义的节点) */
static struct gpio_dt_spec specical_control[3] = {GPIO_DT_SPEC_GET(DT_NODELABEL(suction), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(paper_feed), gpios),
                                                  GPIO_DT_SPEC_GET(DT_NODELABEL(flush), gpios)};

static struct gpio_dt_spec specical_key[3] = {GPIO_DT_SPEC_GET(DT_NODELABEL(sw_suction), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(sw_paper_feed), gpios),
                                              GPIO_DT_SPEC_GET(DT_NODELABEL(sw_flush), gpios)};

/* LED 引脚定义 */
static struct gpio_dt_spec led1         = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led2         = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static struct gpio_dt_spec led_capslock = GPIO_DT_SPEC_GET(DT_NODELABEL(led_capslock), gpios);
/**
 * @brief 初始化GPIO控制模块
 * @retval None
 */
void GPIO_Control_Init(void)
{
    int ret;

    // 对于STM32F103，需要禁用JTAG/SWD功能以允许PB4用作普通GPIO
    // 通过AFIO的SWJ_CFG位来禁用JTAG功能
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();  // 只禁用JTAG，保留SWD

    // 检查GPIO设备是否就绪
    for (int i = 0; i < 6; i++)
    {
        if (!device_is_ready(kb_rows[i].port))
        {
            printk("Keyboard row %d GPIO device not ready\n", i);
            return;
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (!device_is_ready(kb_cols[i].port))
        {
            printk("Keyboard col %d GPIO device not ready\n", i);
            return;
        }
    }

    if (!device_is_ready(led_capslock.port))
    {
        printk("CAPSLOCK GPIOC device not ready\n");
        return;
    }

    // 初始化键盘行控制引脚 (输出) - PB8-PB13
    for (int i = 0; i < 6; i++)
    {
        ret = gpio_pin_configure_dt(&kb_rows[i], GPIO_OUTPUT_HIGH);
        if (ret < 0)
        {
            printk("Failed to configure Row %d GPIO pin (err %d)\n", i, ret);
            return;
        }
    }

    // 初始化键盘列读取引脚 (输入) - PA0-PA7
    for (int i = 0; i < 8; i++)
    {
        ret = gpio_pin_configure_dt(&kb_cols[i], GPIO_INPUT);
        if (ret < 0)
        {
            printk("Failed to configure Col %d GPIO pin (err %d)\n", i, ret);
            return;
        }
    }


    // 初始化特殊按键输出引脚
    for (int i = 0; i < 3; i++)
    {
        if (!device_is_ready(specical_control[i].port))
        {
            printk("Special key output GPIO device %d not ready\n", i);
            return;
        }
        ret = gpio_pin_configure_dt(&specical_control[i], GPIO_OUTPUT_HIGH);
        if (ret < 0)
        {
            printk("Failed to configure special key output %d (err %d)\n", i, ret);
            return;
        }
    }

    // 初始化特殊按键输入引脚
    for (int i = 0; i < 3; i++)
    {
        if (!device_is_ready(specical_key[i].port))
        {
            printk("Special key input GPIO device %d not ready\n", i);
            return;
        }
        ret = gpio_pin_configure_dt(&specical_key[i], GPIO_INPUT);
        if (ret < 0)
        {
            printk("Failed to configure special key input %d (err %d)\n", i, ret);
            return;
        }
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

    // 初始化CAPSLOCK引脚
    if (!device_is_ready(led_capslock.port))
    {
        printk("CAPSLOCK GPIOC device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&led_capslock, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        printk("Failed to configure CAPSLOCK GPIO pin (err %d)\n", ret);
        return;
    }

    printk("GPIO control module initialized successfully\n");
}

/**
 * @brief 设置键盘行的状态
 * @param row_pattern: 行模式 (位0-5对应PB8-PB13)
 * @retval None
 */
void GPIO_SetKeyRows(uint8_t row_pattern)
{
    gpio_pin_set_dt(&kb_rows[0], (row_pattern & 0x01) ? 1 : 0);   // Row 0 - PB8
    gpio_pin_set_dt(&kb_rows[1], (row_pattern & 0x02) ? 1 : 0);   // Row 1 - PB9
    gpio_pin_set_dt(&kb_rows[2], (row_pattern & 0x04) ? 1 : 0);   // Row 2 - PB10
    gpio_pin_set_dt(&kb_rows[3], (row_pattern & 0x08) ? 1 : 0);   // Row 3 - PB11
    gpio_pin_set_dt(&kb_rows[4], (row_pattern & 0x10) ? 1 : 0);   // Row 4 - PB12
    gpio_pin_set_dt(&kb_rows[5], (row_pattern & 0x20) ? 1 : 0);   // Row 5 - PB13
}

/**
 * @brief 设置单个键盘行的状态
 * @param row: 行号 (0-5对应PB8-PB13)
 * @param state: 状态 (0=低电平, 1=高电平)
 * @retval None
 */
void GPIO_SetKeyRow(uint8_t row, uint8_t state)
{
    if (row < 6)
    {
        gpio_pin_set_dt(&kb_rows[row], state ? 1 : 0);
    }
}

/**
 * @brief 读取所有键盘列的状态
 * @retval uint8_t: 列状态 (位0-7对应PA0-PA7)
 */
uint8_t GPIO_ReadKeyCols(void)
{
    uint8_t col_state = 0;

    if (gpio_pin_get_dt(&kb_cols[0]) == 0)  // Col 0 - PA0, 低电平表示按键按下
        col_state |= 0x01;
    if (gpio_pin_get_dt(&kb_cols[1]) == 0)  // Col 1 - PA1
        col_state |= 0x02;
    if (gpio_pin_get_dt(&kb_cols[2]) == 0)  // Col 2 - PA2
        col_state |= 0x04;
    if (gpio_pin_get_dt(&kb_cols[3]) == 0)  // Col 3 - PA3
        col_state |= 0x08;
    if (gpio_pin_get_dt(&kb_cols[4]) == 0)  // Col 4 - PA4
        col_state |= 0x10;
    if (gpio_pin_get_dt(&kb_cols[5]) == 0)  // Col 5 - PA5
        col_state |= 0x20;
    if (gpio_pin_get_dt(&kb_cols[6]) == 0)  // Col 6 - PA6
        col_state |= 0x40;
    if (gpio_pin_get_dt(&kb_cols[7]) == 0)  // Col 7 - PA7
        col_state |= 0x80;

    return col_state;
}

/**
 * @brief 读取单个键盘列的状态
 * @param col: 列号 (0-7对应PA0-PA7)
 * @retval uint8_t: 列状态 (0=高电平, 1=低电平)
 */
uint8_t GPIO_ReadKeyCol(uint8_t col)
{
    if (col < 8)
    {
        int pin_state = gpio_pin_get_dt(&kb_cols[col]);
        return (pin_state == 0) ? 1 : 0;  // 0表示低电平(按键按下)，1表示高电平(按键释放)
    }

    return 1;  // 默认返回未按下状态
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
 * @brief 设置CAPSLOCK LED状态
 * @param state: 状态 (0=熄灭, 1=点亮)
 * @retval None
 */
void GPIO_SetCapsLock(uint8_t state)
{
    gpio_pin_set_dt(&led_capslock, state ? 1 : 0);  // PC15
}

/**
 * @brief 切换CAPSLOCK LED状态
 * @retval None
 */
void GPIO_ToggleCapsLock(void)
{
    int ret = gpio_pin_toggle_dt(&led_capslock);  // PC15
    if (ret < 0)
    {
        printk("Failed to toggle CAPSLOCK (err %d)\n", ret);
    }
}

/**
 * @brief 读取CAPSLOCK状态
 * @retval uint8_t: 状态 (0=高电平, 1=低电平)
 */
uint8_t GPIO_ReadCapsLock(void)
{
    return gpio_pin_get_dt(&led_capslock) ? 1 : 0;  // PC15
}

/**
 * @brief 设置特殊按键输出引脚状态
 * @param key_type: 按键类型 (0=吸液键, 1=走纸键, 2=冲洗键)
 * @param state: 状态 (0=低电平, 1=高电平)
 * @retval None
 */
void GPIO_SetSpecialKeyOutput(uint8_t key_type, uint8_t state)
{
    if (key_type < 3 && device_is_ready(specical_control[key_type].port))
    {
        gpio_pin_set_dt(&specical_control[key_type], state ? 1 : 0);
    }
}

/**
 * @brief 读取特殊按键输入引脚状态
 * @param key_type: 按键类型 (0=吸液键, 1=走纸键, 2=冲洗键)  
 * @retval uint8_t: 引脚状态 (0=高电平, 1=低电平)
 */
uint8_t GPIO_ReadSpecialKeyInput(uint8_t key_type)
{
    if (key_type < 3 && device_is_ready(specical_key[key_type].port))
    {
        int pin_state = gpio_pin_get_dt(&specical_key[key_type]);
        return (pin_state == 0) ? 1 : 0;  // 0表示低电平(按键按下)，1表示高电平(按键释放)
    }

    return 0;  // 默认返回未按下状态
}

/**
 * @brief 扫描键盘状态
 * @retval uint16_t: 键盘扫描码
 */
uint16_t GPIO_ScanKeyboard(void)
{
    uint16_t key_code = 0;

    // 扫描每一行 (PB8-PB13)
    for (uint8_t row = 0; row < 6; row++)
    {
        // 设置当前行为低电平，其余为高电平
        GPIO_SetKeyRows(~(1 << row));

        // 短暂延时以稳定信号
        k_msleep(1);

        // 读取列状态 (PA0-PA7)
        uint8_t col_state = GPIO_ReadKeyCols();

        // 如果有按键按下，记录行和列
        if (col_state != 0xFF)
        {  // 0xFF表示所有列都是高电平(无按键)
            for (uint8_t col = 0; col < 8; col++)
            {
                if ((col_state & (1 << col)) == 0)
                {  // 检测到低电平(按键按下)
                    // 计算键码 (行*8 + 列)
                    key_code = (row << 8) | col;
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

    // 恢复所有行为高电平
    GPIO_SetKeyRows(0xFF);

    return key_code;
}