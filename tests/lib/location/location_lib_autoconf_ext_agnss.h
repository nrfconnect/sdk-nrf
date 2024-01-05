/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Cause nrf/lib/location to act as though these configs are set, even though they are not.
 * (They are set as follows because we are mocking respective APIs)
 */
#define CONFIG_NRF_CLOUD_AGNSS 1
#define CONFIG_NRF_CLOUD_REST 1
#define CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE 1700

#define CONFIG_NET_PKT_TIMESTAMP 1
#define CONFIG_NET_NATIVE 1
