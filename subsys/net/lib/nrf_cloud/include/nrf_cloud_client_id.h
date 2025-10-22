/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#ifndef NRF_CLOUD_CLIENT_ID_H_
#define NRF_CLOUD_CLIENT_ID_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get the pointer to the device ID string. */
int nrf_cloud_client_id_ptr_get(const char **client_id);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CLIENT_ID_H_ */
