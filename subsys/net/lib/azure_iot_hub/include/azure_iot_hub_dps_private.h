/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AZURE_IOT_HUB_DPS_PRIVATE__
#define AZURE_IOT_HUB_DPS_PRIVATE__

#include <net/azure_iot_hub.h>
#include <azure/az_core.h>
#include <azure/az_iot.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AZURE_IOT_HUB_DPS_REG_STATUS_UPDATE_MAX_RETRIES	10
#define DPS_OPERATION_ID_MAX_LEN			60
#define DPS_REGISTERED_HUB_MAX_LEN			100
#define DPS_SETTINGS_KEY				"azure_iot_hub"
#define DPS_SETTINGS_HOSTNAME_LEN_KEY			"hostname_len"
#define DPS_SETTINGS_HOSTNAME_KEY			"hostname"
#define DPS_SETTINGS_DEVICE_ID_LEN_KEY			"device_id_len"
#define DPS_SETTINGS_DEVICE_ID_KEY			"device_id"


#define RETRY_AFTER_SEC_DEFAULT				20

/* This file contains private types and APIs.
 * The public DPS interface is found in include/net/azure_iot_hub_dps.h
 */

enum dps_state {
	/* The library is uninitialized. */
	DPS_STATE_UNINIT,
	/* The library is initialized, no connection established. */
	DPS_STATE_DISCONNECTED,
	/* Connecting to Azure IoT Hub. */
	DPS_STATE_CONNECTING,
	/* Connected to Azure IoT Hub on TLS level. */
	DPS_STATE_TRANSPORT_CONNECTED,
	/* Connected to Azure IoT Hub on MQTT level. */
	DPS_STATE_CONNECTED,
	/* Disconnecting from Azure IoT Hub. */
	DPS_STATE_DISCONNECTING,

	DPS_STATE_COUNT,
};

struct dps_reg_ctx {
	bool is_init;
	enum azure_iot_hub_dps_reg_status status;
	struct k_sem settings_loaded;
	struct k_work_delayable timeout_work;

	/* The number of times the status has been polled without registration being completed. */
	uint32_t retry_count;

	azure_iot_hub_dps_handler_t cb;

	az_iot_provisioning_client dps_client;
	az_span id_scope;
	az_span reg_id;
	az_span operation_id;
	az_span assigned_hub;
	az_span assigned_device_id;
};

#ifdef __cplusplus
}
#endif

#endif /* AZURE_IOT_HUB_DPS_PRIVATE__ */
