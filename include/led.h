#ifndef LED_H
#define LED_H

#include <stdint.h>

#define LED_COUNT 4

/**
 * @brief Initialize all LEDs
 * @return 0 on success, -1 on failure
 */
int led_init(void);

/**
 * @brief Set state of a single LED
 * @param led_index LED index (0-3)
 * @param state 0 for off, 1 for on
 */
void led_set_state(uint8_t led_index, int state);

/**
 * @brief Set state of all LEDs
 * @param state 0 for off, 1 for on
 */
void led_set_all(int state);

/**
 * @brief Set only one LED on, all others off
 * @param led_index LED index (0-3) to turn on
 */
void led_set_single(uint8_t led_index);

/**
 * @brief Set LED pattern using bit mask
 * @param pattern Bit mask where bit i controls LED i
 */
void led_set_pattern(uint8_t pattern);

#endif /* LED_H */