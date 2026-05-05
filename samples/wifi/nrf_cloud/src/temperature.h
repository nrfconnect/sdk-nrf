/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_

/**
 * @brief Take a fake temperature sample.
 *
 * Returns a pseudorandom dummy temperature reading.
 *
 * @param[out] temp - Pointer to the double to be filled with the taken temperature sample.
 * @return int - 0 on success, otherwise, negative error code.
 */
int get_temperature(double *temp);

#endif /* _TEMPERATURE_H_ */
