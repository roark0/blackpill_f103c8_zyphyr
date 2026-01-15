#ifndef DISPLAY_H
#define DISPLAY_H

/**
 * @brief Initialize 7-segment display GPIO
 * @return 0 on success, -1 on failure
 */
int display_init(void);

/**
 * @brief Display temperature on 7-segment display
 * @param temperature Temperature value to display
 */
void display_temp(float temperature);

#endif /* DISPLAY_H */