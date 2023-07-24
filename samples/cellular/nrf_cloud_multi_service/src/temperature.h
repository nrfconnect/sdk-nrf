/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_

/**
 * @brief Take a temperature sample.
 *
 * Value will either be a real sensor value taken from a sensor with device tree alias temp_sensor,
 * (if CONFIG_TEMP_DATA_USE_SENSOR is set) or will otherwise be a pseudorandom dummy reading.
 *
 * @param[out] temp - Pointer to the double to be filled with the taken temperature sample.
 * @return int - 0 on success, otherwise, negative error code.
 */
int get_temperature(double *temp);

#endif /* _TEMPERATURE_H_ */
