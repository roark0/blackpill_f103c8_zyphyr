#include "gpio_control.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include <stm32f1xx_hal.h>

/* 定义键盘行和列的GPIO引脚 (直接使用GPIO端口和引脚号) */
static const struct device *kb_row_port = DEVICE_DT_GET(DT_NODELABEL(gpiob));  // GPIOB for keyboard row pins (PB8-PB13)
static const struct device *kb_col_port = DEVICE_DT_GET(DT_NODELABEL(gpioa));  // GPIOA for keyboard column pins (PA0-PA7)
// static const struct device *capslock_port = DEVICE_DT_GET(DT_NODELABEL(gpioc));  // GPIOC for CAPSLOCK pin (PC15)

/* 定义特殊按键GPIO引脚 (使用设备树定义的节点) */
static struct gpio_dt_spec suction = GPIO_DT_SPEC_GET(DT_NODELABEL(suction), gpios);
static struct gpio_dt_spec suction_key_input = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_suction), gpios);
static struct gpio_dt_spec paper_feed = GPIO_DT_SPEC_GET(DT_NODELABEL(paper_feed), gpios);
static struct gpio_dt_spec sw_paper_feed = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_paper_feed), gpios);
static struct gpio_dt_spec flush = GPIO_DT_SPEC_GET(DT_NODELABEL(flush), gpios);
static struct gpio_dt_spec sw_flush = GPIO_DT_SPEC_GET(DT_NODELABEL(sw_flush), gpios);

/* LED 引脚定义 */
static struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
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
    if (!device_is_ready(kb_row_port))
    {
        printk("Keyboard GPIOB device not ready\n");
        return;
    }
    
    if (!device_is_ready(kb_col_port))
    {
        printk("Keyboard GPIOA device not ready\n");
        return;
    }
    
    if (!device_is_ready(led_capslock.port))
    {
        printk("CAPSLOCK GPIOC device not ready\n");
        return;
    }
    
    // 初始化键盘行控制引脚 (输出) - PB8-PB13
    for (int i = 8; i <= 13; i++)
    {
        ret = gpio_pin_configure(kb_row_port, i, GPIO_OUTPUT_HIGH);
        if (ret < 0)
        {
            printk("Failed to configure Row %d (PB%d) GPIO pin (err %d)\n", i-8, i, ret);
            return;
        }
    }

    // 初始化键盘列读取引脚 (输入) - PA0-PA7
    for (int i = 0; i <= 7; i++)
    {
        ret = gpio_pin_configure(kb_col_port, i, GPIO_INPUT);
        if (ret < 0)
        {
            printk("Failed to configure Col %d (PA%d) GPIO pin (err %d)\n", i, i, ret);
            return;
        }
    }

    // 初始化吸液键引脚
    if (!device_is_ready(suction.port))
    {
        printk("Suction key output GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&suction, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure suction key output (err %d)\n", ret);
        return;
    }
    
    if (!device_is_ready(suction_key_input.port))
    {
        printk("Suction key input GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&suction_key_input, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure suction key input (err %d)\n", ret);
        return;
    }
    
    // 初始化走纸键引脚
    if (!device_is_ready(paper_feed.port))
    {
        printk("Paper feed key output GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&paper_feed, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure paper feed key output (err %d)\n", ret);
        return;
    }
    
    if (!device_is_ready(sw_paper_feed.port))
    {
        printk("Paper feed key input GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&sw_paper_feed, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure paper feed key input (err %d)\n", ret);
        return;
    }
    
    // 初始化冲洗键引脚
    if (!device_is_ready(flush.port))
    {
        printk("Flush key output GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&flush, GPIO_OUTPUT_HIGH);
    if (ret < 0)
    {
        printk("Failed to configure flush key output (err %d)\n", ret);
        return;
    }
    
    if (!device_is_ready(sw_flush.port))
    {
        printk("Flush key input GPIO device not ready\n");
        return;
    }
    ret = gpio_pin_configure_dt(&sw_flush, GPIO_INPUT);
    if (ret < 0)
    {
        printk("Failed to configure flush key input (err %d)\n", ret);
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
    gpio_pin_set(kb_row_port, 8, (row_pattern & 0x01) ? 1 : 0);  // Row 0 - PB8
    gpio_pin_set(kb_row_port, 9, (row_pattern & 0x02) ? 1 : 0);  // Row 1 - PB9
    gpio_pin_set(kb_row_port, 10, (row_pattern & 0x04) ? 1 : 0);  // Row 2 - PB10
    gpio_pin_set(kb_row_port, 11, (row_pattern & 0x08) ? 1 : 0);  // Row 3 - PB11
    gpio_pin_set(kb_row_port, 12, (row_pattern & 0x10) ? 1 : 0);  // Row 4 - PB12
    gpio_pin_set(kb_row_port, 13, (row_pattern & 0x20) ? 1 : 0);  // Row 5 - PB13
}


/**
 * @brief 设置单个键盘行的状态
 * @param row: 行号 (0-5对应PB8-PB13)
 * @param state: 状态 (0=低电平, 1=高电平)
 * @retval None
 */
void GPIO_SetKeyRow(uint8_t row, uint8_t state)
{
    switch (row)
    {
        case 0:
            gpio_pin_set(kb_row_port, 8, state ? 1 : 0);  // Row 0 - PB8
            break;
        case 1:
            gpio_pin_set(kb_row_port, 9, state ? 1 : 0);  // Row 1 - PB9
            break;
        case 2:
            gpio_pin_set(kb_row_port, 10, state ? 1 : 0);  // Row 2 - PB10
            break;
        case 3:
            gpio_pin_set(kb_row_port, 11, state ? 1 : 0);  // Row 3 - PB11
            break;
        case 4:
            gpio_pin_set(kb_row_port, 12, state ? 1 : 0);  // Row 4 - PB12
            break;
        case 5:
            gpio_pin_set(kb_row_port, 13, state ? 1 : 0);  // Row 5 - PB13
            break;
        default:
            break;
    }
}

/**
 * @brief 读取所有键盘列的状态
 * @retval uint8_t: 列状态 (位0-7对应PA0-PA7)
 */
uint8_t GPIO_ReadKeyCols(void)
{
    uint8_t col_state = 0;

    if (gpio_pin_get(kb_col_port, 0) == 0)  // Col 0 - PA0, 低电平表示按键按下
        col_state |= 0x01;
    if (gpio_pin_get(kb_col_port, 1) == 0)  // Col 1 - PA1
        col_state |= 0x02;
    if (gpio_pin_get(kb_col_port, 2) == 0)  // Col 2 - PA2
        col_state |= 0x04;
    if (gpio_pin_get(kb_col_port, 3) == 0)  // Col 3 - PA3
        col_state |= 0x08;
    if (gpio_pin_get(kb_col_port, 4) == 0)  // Col 4 - PA4
        col_state |= 0x10;
    if (gpio_pin_get(kb_col_port, 5) == 0)  // Col 5 - PA5
        col_state |= 0x20;
    if (gpio_pin_get(kb_col_port, 6) == 0)  // Col 6 - PA6
        col_state |= 0x40;
    if (gpio_pin_get(kb_col_port, 7) == 0)  // Col 7 - PA7
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
    int pin_state = 1;  // 默认为高电平

    switch (col)
    {
        case 0:
            pin_state = gpio_pin_get(kb_col_port, 0);  // Col 0 - PA0
            break;
        case 1:
            pin_state = gpio_pin_get(kb_col_port, 1);  // Col 1 - PA1
            break;
        case 2:
            pin_state = gpio_pin_get(kb_col_port, 2);  // Col 2 - PA2
            break;
        case 3:
            pin_state = gpio_pin_get(kb_col_port, 3);  // Col 3 - PA3
            break;
        case 4:
            pin_state = gpio_pin_get(kb_col_port, 4);  // Col 4 - PA4
            break;
        case 5:
            pin_state = gpio_pin_get(kb_col_port, 5);  // Col 5 - PA5
            break;
        case 6:
            pin_state = gpio_pin_get(kb_col_port, 6);  // Col 6 - PA6
            break;
        case 7:
            pin_state = gpio_pin_get(kb_col_port, 7);  // Col 7 - PA7
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
 * @brief 设置CAPSLOCK LED状态
 * @param state: 状态 (0=熄灭, 1=点亮)
 * @retval None
 */
void GPIO_SetCapsLock(uint8_t state)
{
    gpio_pin_set_dt(&led_capslock,  state ? 1 : 0);  // PC15
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
    const struct gpio_dt_spec *output_spec = NULL;
    
    switch (key_type)
    {
        case 0:  // 吸液键
            output_spec = &suction;
            break;
        case 1:  // 走纸键
            output_spec = &paper_feed;
            break;
        case 2:  // 冲洗键
            output_spec = &flush;
            break;
        default:
            break;
    }
    
    if (output_spec != NULL && device_is_ready(output_spec->port))
    {
        gpio_pin_set_dt(output_spec, state ? 1 : 0);
    }
}

/**
 * @brief 读取特殊按键输入引脚状态
 * @param key_type: 按键类型 (0=吸液键, 1=走纸键/冲洗键)  
 * @retval uint8_t: 引脚状态 (0=高电平, 1=低电平)
 */
uint8_t GPIO_ReadSpecialKeyInput(uint8_t key_type)
{
    const struct gpio_dt_spec *input_spec = NULL;
    
    switch (key_type)
    {
        case 0:  // 吸液键 (PB2 - KSNS2)
            input_spec = &suction_key_input;
            break;
        case 1:  // 走纸键/冲洗键 (PB1 - KSNS1)
            // 走纸键和冲洗键共用KSNS1，所以需要根据具体情况选择
            // 通常在执行走纸键或冲洗键检测时调用
            input_spec = &sw_paper_feed;  // 或 &sw_flush，共用同一个引脚
            break;
        default:
            break;
    }
    
    if (input_spec != NULL && device_is_ready(input_spec->port))
    {
        int pin_state = gpio_pin_get_dt(input_spec);
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