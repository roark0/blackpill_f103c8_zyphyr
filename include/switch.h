#ifndef SWITCH_H
#define SWITCH_H

#include <stdbool.h>

#define SWITCH_COUNT 4

/**
 * @brief Initialize all switches
 * @return 0 on success, -1 on failure
 */
int switch_init(void);

/**
 * @brief Read switch settings
 * @return Temperature offset value with sign
 */
float switch_read_settings(void);

/**
 * @brief Initialize button interrupt
 * @return 0 on success, -1 on failure
 */
int button_init(void);

#endif /* SWITCH_H */