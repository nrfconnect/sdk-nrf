/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_API_H__
#define __OSAL_API_H__

#include "osal_structs.h"

/* Have to match zephyr/include/zephyr/logging/log_core.h */
#define NRF_WIFI_LOG_LEVEL_ERR 1U
#define NRF_WIFI_LOG_LEVEL_INF 3U
#define NRF_WIFI_LOG_LEVEL_DBG 4U

#ifndef WIFI_NRF71_LOG_LEVEL
#define WIFI_NRF71_LOG_LEVEL NRF_WIFI_LOG_LEVEL_ERR
#endif

#ifndef NRF71_LOG_VERBOSE
#define __func__ "<snipped>"
#endif /* NRF71_LOG_VERBOSE */

/**
 * @brief Initialize the OSAL layer.
 * @param ops: Pointer to the OSAL operations structure.
 *
 * Initializes the OSAL layer and is expected to be called
 * before using the OSAL layer.
 */
void nrf_wifi_osal_init(const struct nrf_wifi_osal_ops *ops);

/**
 * @brief Deinitialize the OSAL layer.
 *
 * Deinitialize the OSAL layer and is expected to be called after done using
 * the OSAL layer.
 */
void nrf_wifi_osal_deinit(void);

/**
 * nrf_wifi_osal_rand8_get() - Get a random 8 bit number.
 *
 * Generates an 8 bit random number.
 *
 * Return: an 8 bit random number.
 */
unsigned char nrf_wifi_osal_rand8_get(void);

#endif /* __OSAL_API_H__ */
