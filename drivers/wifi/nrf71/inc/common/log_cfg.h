/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Compile-time log configuration for the nRF71 Wi-Fi driver.
 */

#ifndef __LOG_CFG_H__
#define __LOG_CFG_H__

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

#endif /* __LOG_CFG_H__ */
