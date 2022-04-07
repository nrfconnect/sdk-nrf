/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _POWER_MODULE_H_
#define _POWER_MODULE_H_

#include <zephyr.h>
#include <stdlib.h>

#include "ina231.h"

#define RAIL_NAME_MAX_SIZE 20

typedef void (*nrf_power_module_handler_t)(uint8_t rail);

/*
 * Different rails to be measured by the INA231
 * VBAT		= Battery
 * VDD1_CODEC	= 1.2V rail for CS47L63
 * VDD2_CODEC	= 1.8V rail for CS47L63
 * VDD2_NRF	= NRF5340
 */
enum ina_name {
	VBAT,
	VDD1_CODEC,
	VDD2_CODEC,
	VDD2_NRF,
};

struct power_module_data {
	float current;
	float power;
	float bus_voltage;
	float shunt_voltage;
};

/**@brief   Read the latest measurements from a given INA231
 *
 * @param   name    Name of the INA231 to be read from
 * @param   data    Container for the read data
 */
void power_module_data_get(enum ina_name name, struct power_module_data *data);

/**@brief   Write configuration to INA231 to start measurements
 *
 * @param   name    Name of the INA231 to start
 *
 * @param   data_handler    Callback to use when data is ready, if set to NULL
 *			    the default callback for printing measurements will
 *			    be used
 *
 * @return  0 if successful
 */
int power_module_measurement_start(enum ina_name name, nrf_power_module_handler_t data_handler);

/**@brief   Stop continuous measurements from given INA231
 *
 * @param   name    Name of the INA231 to be stopped
 *
 * @return  0 if successful
 */
int power_module_measurement_stop(enum ina_name name);

/**@brief   Getter for average value of the INAs
 *
 * @return  value of average setting
 */
uint8_t power_module_avg_get(void);

/**@brief   Setter for average value of the INAs
 *
 * @param   avg     value of average setting
 *
 * @note A given INA must be restarted for the new value to take effect.
 *
 * @return  0 if successful
 */
int power_module_avg_set(uint8_t avg);

/**@brief   Getter for conversion time value of the INAs
 *
 * @return  value of conversion time setting
 */
uint8_t power_module_conv_time_get(void);

/**@brief   Setter for conversion time value of the INAs
 *
 * @param   conv_time     value of conversion time setting
 *
 * @note A given INA must be restarted for the new value to take effect.
 *
 * @return  0 if successful
 */
int power_module_conv_time_set(uint8_t conv_time);

/**@brief   Initialize power module
 *
 * @return  0 if successful
 */
int power_module_init(void);

#endif /* _POWER_MODULE_H_ */
