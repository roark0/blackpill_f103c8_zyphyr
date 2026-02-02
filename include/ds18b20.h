#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

/**
 * @brief Initialize DS18B20 sensor
 * @return 0 on success, -1 on failure
 */
int ds18b20_init(void);

/**
 * @brief Read temperature from DS18B20
 * @param temp_offset Temperature offset to apply
 * @return Temperature value in Celsius with offset applied
 */
float ds18b20_read_temperature(float temp_offset);

/**
 * @brief Run DS18B20 diagnostic test
 */
void ds18b20_diagnostic(void);

#endif /* DS18B20_H */