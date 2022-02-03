/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <drivers/sensor.h>

/* Resets measurement data and initializes Bosch BME688 sensor */
void sensor_init(void);

/* Fetches sensor sample and stores temperature, pressure and humidity data */
void sensor_update(void);

/* Returns temperature measurement fetched with last sensor_update call, converted to float */
float sensor_get_temperature(void);

/* Returns pressure measurement fetched with last sensor_update call, converted to float */
float sensor_get_pressure(void);

/* Returns relative humidity measurement fetched with last sensor_update call, converted to float */
float sensor_get_humidity(void);

#endif /* SENSOR_H */
