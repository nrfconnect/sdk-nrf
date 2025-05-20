/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LIGHT_SENSOR_H__
#define LIGHT_SENSOR_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read the R, G, B, and IR channels of the light sensor.
 *
 * @param[out] light_value 4-byte representation of light values.
 *             Each byte represents a different colour channel.
 *             MSB=R->G->B->IR=LSB.
 * @return int 0 if successful, negative error code if not.
 *         Returns -EBUSY if sample fetch failed; user should wait some hundred
 *         milliseconds before trying again.
 */
int light_sensor_read(uint32_t *light_value);

/**
 * @brief Read the R, G, B, and IR channels of the light sensor while the
 *        sense LED is on. Used to measure the colour of a surface.
 *
 * @param[out] light_value 4-byte representation of colour values.
 *             Each byte represents a different colour channel.
 *             MSB=R->G->B->IR=LSB.
 * @return int 0 if successful, negative error code if not.
 *         Returns -EBUSY if sample fetch failed; user should wait some hundred
 *         milliseconds before trying again.
 */
int colour_sensor_read(uint32_t *colour_value);

/**
 * @brief Initialize the light sensor and apply the trigger settings set in
 *        menuconfig/guiconfig.
 *
 * @return int 0 if successful, negative error code if not.
 */
int light_sensor_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_SENSOR_H__ */
