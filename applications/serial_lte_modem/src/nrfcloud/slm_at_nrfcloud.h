/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_NRFCLOUD_
#define SLM_AT_NRFCLOUD_

/** @file slm_at_nrfcloud.h
 *
 * @brief Vendor-specific AT command for nRF Cloud service.
 * @{
 */
#if defined(CONFIG_SLM_NRF_CLOUD)

#include <stdbool.h>

/* Whether the connection to nRF Cloud is ready. */
extern bool slm_nrf_cloud_ready;

/* Whether to send the device's location to nRF Cloud. */
extern bool slm_nrf_cloud_send_location;

/**
 * @brief Initialize nRF Cloud AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_nrfcloud_init(void);

/**
 * @brief Uninitialize nRF Cloud AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_nrfcloud_uninit(void);

#endif /* CONFIG_SLM_NRF_CLOUD */
/** @} */
#endif /* SLM_AT_NRFCLOUD_ */
