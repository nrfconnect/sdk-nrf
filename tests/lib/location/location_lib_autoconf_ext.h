/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Cause nrf/lib/location to act as though these configs are set, even though they are not.
 * (They are set as follows because we are mocking respective APIs)
 */
#define CONFIG_NRF_MODEM_LIB 1

#define CONFIG_WIFI_NRF70 1
#define CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS 0
#define CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH 10
#define CONFIG_NET_MGMT_EVENT 1
#define CONFIG_NET_MGMT_EVENT_INFO 1
