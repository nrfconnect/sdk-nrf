/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing common status declarations for the nRF71 driver.
 */

#ifndef __COMMON_STATUS_H__
#define __COMMON_STATUS_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

/**
 * @brief The status of an operation performed by the nRF71 driver.
 */
enum nrf_wifi_status {
	/** The operation was successful. */
	NRF_WIFI_STATUS_SUCCESS,
	/** The operation failed. */
	NRF_WIFI_STATUS_FAIL = -1
};

#endif /* __COMMON_STATUS_H__ */
