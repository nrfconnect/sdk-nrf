/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_CREDENTIALS_H_
#define NRF_CLOUD_CREDENTIALS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Provision nRF Cloud credentials to the device.
 *
 * @retval 0 If successful.
 * @return A negative value indicates an error.
 */
int nrf_cloud_credentials_provision(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_CREDENTIALS_H_ */
