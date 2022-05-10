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

/** @brief Gets the length of the Kconfig configured client id string. */
size_t nrf_cloud_configured_client_id_length_get(void);

/** @brief Copies the Kconfig configured client id string to the provided buffer */
int nrf_cloud_configured_client_id_get(char * const buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CLIENT_ID_H_ */
