/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <net/nrf_cloud.h>
#include <nrf_cloud_fsm.h>
#include <zephyr/shell/shell.h>

#if defined(CONFIG_SAMPLE_DESH_NATIVE_TLS) && !defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
#include "desh_native_tls.h"
#endif
#include "desh_print.h"

#define CLOUD_CMD_MAX_LENGTH 150

BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_MQTT));
BUILD_ASSERT(!IS_ENABLED(CONFIG_NRF_CLOUD_COAP));

extern const struct shell *desh_shell;
extern struct k_work_q desh_common_work_q;

static struct k_work_delayable cloud_reconnect_work;
static struct k_work cloud_cmd_execute_work;
static struct k_work shadow_update_work;

static char shell_cmd[CLOUD_CMD_MAX_LENGTH + 1];
static void cmd_cloud_disconnect(const struct shell *shell, size_t argc, char **argv);

static void cloud_reconnect_work_fn(struct k_work *work)
{
	int err = nrf_cloud_connect();

	if (err == NRF_CLOUD_CONNECT_RES_SUCCESS) {
		desh_print("Connecting to nRF Cloud...");
	} else if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		desh_print("nRF Cloud connection already established");
	} else {
		desh_error("nrf_cloud_connect, error: %d", err);
	}
}

static K_WORK_DELAYABLE_DEFINE(cloud_reconnect_work, cloud_reconnect_work_fn);

static void cloud_cmd_execute_work_fn(struct k_work *work)
{
	shell_execute_cmd(desh_shell, shell_cmd);
	memset(shell_cmd, 0, CLOUD_CMD_MAX_LENGTH);
}

static K_WORK_DEFINE(cloud_cmd_execute_work, cloud_cmd_execute_work_fn);

static bool cloud_shell_parse_desh_cmd(const char *buf_in)
{
	const cJSON *app_id = NULL;
	const cJSON *desh_cmd = NULL;
	bool ret = false;

	cJSON *cloud_cmd_json = cJSON_Parse(buf_in);

	/* A format example expected from nrf cloud:
	 * {"appId":"DECT_SHELL", "data":"version"}
	 */
	if (cloud_cmd_json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();

		if (error_ptr != NULL) {
			desh_error("JSON parsing error before: %s\n", error_ptr);
		}
		ret = false;
		goto end;
	}

	/* DeSh commands are identified by checking if appId equals "DECT_SHELL" */
	app_id = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json, NRF_CLOUD_JSON_APPID_KEY);
	if (cJSON_IsString(app_id) && (app_id->valuestring != NULL)) {
		if (strcmp(app_id->valuestring, "DECT_SHELL") != 0) {
			ret = false;
			goto end;
		}
	}

	/* The value of attribute "data" contains the actual command */
	desh_cmd = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json, NRF_CLOUD_JSON_DATA_KEY);
	if (cJSON_IsString(desh_cmd) && (desh_cmd->valuestring != NULL)) {
		desh_print("%s", desh_cmd->valuestring);
		if (strlen(desh_cmd->valuestring) <= CLOUD_CMD_MAX_LENGTH) {
			strcpy(shell_cmd, desh_cmd->valuestring);
			ret = true;
		} else {
			desh_error("Received cloud command exceeds maximum permissible length %d",
				   CLOUD_CMD_MAX_LENGTH);
		}
	}
end:
	cJSON_Delete(cloud_cmd_json);
	return ret;
}

/**
 * @brief Updates the nRF Cloud shadow with information about supported capabilities.
 */
static void nrf_cloud_update_shadow_work_fn(struct k_work *work)
{
	int err;
	struct nrf_cloud_svc_info_ui ui_info = {
#if defined(CONFIG_LOCATION)
		.gnss = true, /* Show map on nrf cloud */
#else
		.gnss = false,
#endif
	};
	struct nrf_cloud_svc_info service_info = {.ui = &ui_info};
#if defined(CONFIG_MODEM_INFO)
	struct nrf_cloud_modem_info modem_info = {
		.device = NRF_CLOUD_INFO_SET, .mpi = NULL /* Modem data will be fetched */
	};
#endif
	struct nrf_cloud_device_status device_status = {
#if defined(CONFIG_MODEM_INFO)
		.modem = &modem_info,
#endif
		.svc = &service_info
	};

	ARG_UNUSED(work);
	err = nrf_cloud_shadow_device_status_update(&device_status);
	if (err) {
		shell_error(desh_shell, "Failed to update device shadow, error: %d", err);
	} else {
		shell_print(desh_shell, "Device shadow updated");
	}
}

static K_WORK_DEFINE(shadow_update_work, nrf_cloud_update_shadow_work_fn);

static void nrf_cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		desh_warn("Add the device to nRF Cloud and reconnect");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_USER_ASSOCIATED");
		break;
	case NRF_CLOUD_EVT_READY:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_READY");
		desh_print("Connection to nRF Cloud established");
		k_work_submit_to_queue(&desh_common_work_q, &shadow_update_work);
		break;
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_RX_DATA_GENERAL");
		if (((char *)evt->data.ptr)[0] == '{') {
			/* Check if it's a DeSh command sent from the cloud */
			if (cloud_shell_parse_desh_cmd(evt->data.ptr)) {
				k_work_submit_to_queue(&desh_common_work_q,
						       &cloud_cmd_execute_work);
			}
		}
		break;
	case NRF_CLOUD_EVT_RX_DATA_LOCATION:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_RX_DATA_LOCATION");
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_RX_DATA_SHADOW");
		break;
	case NRF_CLOUD_EVT_PINGRESP:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_PINGRESP");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED: {
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED, status: %d",
			   evt->status);
		desh_print("Connection to nRF Cloud disconnected");
		/* Stop possibly pending reconnection attempt. */
		k_work_cancel_delayable(&cloud_reconnect_work);

		int err = nrf_cloud_disconnect();

		if (err == -EACCES) {
			desh_print("Not connected to nRF Cloud");
		} else if (err) {
			desh_error("nrf_cloud_disconnect, error: %d", err);
		}
		break;
	}
	case NRF_CLOUD_EVT_FOTA_START:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_FOTA_START");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_FOTA_DONE");
		break;
	case NRF_CLOUD_EVT_FOTA_ERROR:
		desh_error("nRF Cloud event: NRF_CLOUD_EVT_FOTA_ERROR");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		desh_error("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR, status: %d",
			   evt->status);
		desh_error("Connecting to nRF Cloud failed");
		break;
	case NRF_CLOUD_EVT_ERROR:
		desh_print("nRF Cloud event: NRF_CLOUD_EVT_ERROR, status: %d", evt->status);
		break;
	default:
		desh_error("Unknown nRF Cloud event type: %d", evt->type);
		break;
	}
}

static void cmd_cloud_connect(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	static bool initialized;

	if (!initialized) {
		struct nrf_cloud_os_mem_hooks hooks = {
			.malloc_fn = k_malloc,
			.calloc_fn = k_calloc,
			.free_fn = k_free,
		};

		struct nrf_cloud_init_param config = {
			.event_handler = nrf_cloud_event_handler,
			.hooks = &hooks,
			.application_version = desh_print_version_str_get(),
		};

#if defined(CONFIG_SAMPLE_DESH_NATIVE_TLS) && !defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
		err = desh_native_tls_load_credentials(CONFIG_NRF_CLOUD_SEC_TAG);
		if (err) {
			desh_error("%s: failed to load credentials, error: %d", (__func__), err);
		}
#endif
		err = nrf_cloud_init(&config);
		if (err == -EACCES) {
			desh_print("nRF Cloud module already initialized");
		} else if (err) {
			desh_error("nrf_cloud_init, error: %d", err);
			return;
		}

		initialized = true;
	}

	k_work_reschedule(&cloud_reconnect_work, K_NO_WAIT);

	desh_print("Endpoint: %s", CONFIG_NRF_CLOUD_HOST_NAME);
}

static void cmd_cloud_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	/* Stop possibly pending reconnection attempt. */
	k_work_cancel_delayable(&cloud_reconnect_work);

	err = nrf_cloud_disconnect();
	if (err == -EACCES) {
		desh_print("Not connected to nRF Cloud");
	} else if (err) {
		desh_error("nrf_cloud_disconnect, error: %d", err);
	}
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cloud,
			       SHELL_CMD_ARG(connect, NULL,
					     "Establish MQTT connection to nRF Cloud.",
					     cmd_cloud_connect, 1, 0),
			       SHELL_CMD_ARG(disconnect, NULL, "Disconnect from nRF Cloud.",
					     cmd_cloud_disconnect, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cloud, &sub_cloud, "MQTT connection to nRF Cloud", desh_print_help_shell);
