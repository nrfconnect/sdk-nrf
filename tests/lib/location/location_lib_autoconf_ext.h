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
#define CONFIG_NRF_CLOUD_COAP 1
/* Required by zephyr/net/coap_client.h when CONFIG_NRF_CLOUD_COAP is set */
#define CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE 48
#define CONFIG_COAP_CLIENT_MESSAGE_SIZE 256
#define CONFIG_COAP_CLIENT_MAX_PATH_LENGTH 64
#define CONFIG_COAP_CLIENT_MAX_EXTRA_OPTIONS 2
#define CONFIG_COAP_CLIENT_MAX_REQUESTS 2
#define CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE 13
