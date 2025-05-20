/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SERVICE_H_
#define SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <dm.h>

/** @brief Distance Measurement update result.
 *
 *  @param addr Bluetooth LE Device Address.
 *
 *  @param result Measurement structure.
 */
void service_distance_measurement_update(const bt_addr_le_t *addr, const struct dm_result *result);

/** @brief Simulation of azimuth and elevation measurements.
 *
 *  @param None
 */
void service_azimuth_elevation_simulation(void);

/** @brief Initialize the Direction and Distance Finding Service.
 *
 *  @param None
 *
 *  @retval 0 if the operation was successful, otherwise a (negative) error code.
 */
int service_ddfs_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_H_ */
