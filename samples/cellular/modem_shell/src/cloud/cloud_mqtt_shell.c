/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <net/nrf_cloud.h>
#include <nrf_cloud_fsm.h>
#include <zephyr/shell/shell.h>
#include "mosh_print.h"

#define CLOUD_CMD_MAX_LENGTH 150

BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) &&
	IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD));

BUILD_ASSERT(!IS_ENABLED(CONFIG_MOSH_CLOUD_LWM2M));

extern struct k_work_q mosh_common_work_q;
extern const struct shell *mosh_shell;

static struct k_work_delayable cloud_reconnect_work;
static struct k_work cloud_cmd_execute_work;
static struct k_work shadow_update_work;

static char shell_cmd[CLOUD_CMD_MAX_LENGTH + 1];

static int cloud_shell_print_usage(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1) {
		mosh_error("%s: subcommand not found", argv[1]);
		ret = -EINVAL;
	}

	shell_help(shell);

	return ret;
}

static void cloud_reconnect_work_fn(struct k_work *work)
{
	int err = nrf_cloud_connect();

	if (err == NRF_CLOUD_CONNECT_RES_SUCCESS) {
		mosh_print("Connecting to nRF Cloud...");
	} else if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		mosh_print("nRF Cloud connection already established");
	} else {
		mosh_error("nrf_cloud_connect, error: %d", err);
	}
}

static K_WORK_DELAYABLE_DEFINE(cloud_reconnect_work, cloud_reconnect_work_fn);

static void cloud_cmd_execute_work_fn(struct k_work *work)
{
	shell_execute_cmd(mosh_shell, shell_cmd);
	memset(shell_cmd, 0, CLOUD_CMD_MAX_LENGTH);
}

static K_WORK_DEFINE(cloud_cmd_execute_work, cloud_cmd_execute_work_fn);

static bool cloud_shell_parse_mosh_cmd(const char *buf_in)
{
	const cJSON *app_id = NULL;
	const cJSON *mosh_cmd = NULL;
	bool ret = false;

	cJSON *cloud_cmd_json = cJSON_Parse(buf_in);

	/* A format example expected from nrf cloud:
	 * {"appId":"MODEM_SHELL", "data":"location get"}
	 */
	if (cloud_cmd_json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();

		if (error_ptr != NULL) {
			mosh_error("JSON parsing error before: %s\n", error_ptr);
		}
		ret = false;
		goto end;
	}

	/* MoSh commands are identified by checking if appId equals "MODEM_SHELL" */
	app_id = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json, NRF_CLOUD_JSON_APPID_KEY);
	if (cJSON_IsString(app_id) && (app_id->valuestring != NULL)) {
		if (strcmp(app_id->valuestring, "MODEM_SHELL") != 0) {
			ret = false;
			goto end;
		}
	}

	/* The value of attribute "data" contains the actual command */
	mosh_cmd = cJSON_GetObjectItemCaseSensitive(cloud_cmd_json, NRF_CLOUD_JSON_DATA_KEY);
	if (cJSON_IsString(mosh_cmd) && (mosh_cmd->valuestring != NULL)) {
		mosh_print("%s", mosh_cmd->valuestring);
		if (strlen(mosh_cmd->valuestring) <= CLOUD_CMD_MAX_LENGTH) {
			strcpy(shell_cmd, mosh_cmd->valuestring);
			ret = true;
		} else {
			mosh_error("Received cloud command exceeds maximum permissible length %d",
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
static void shadow_update_work_fn(struct k_work *work)
{
	int err;
	struct nrf_cloud_svc_info_ui ui_info = {
		.gnss = IS_ENABLED(CONFIG_MOSH_LOCATION), /* Show map on nrf cloud */
	};
	struct nrf_cloud_svc_info service_info = {
		.ui = &ui_info
	};
	struct nrf_cloud_modem_info modem_info = {
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = NRF_CLOUD_INFO_SET,
		.mpi = NULL /* Modem data will be fetched */
	};
	struct nrf_cloud_device_status device_status = {
		.modem = &modem_info,
		.svc = &service_info
	};

	ARG_UNUSED(work);
	err = nrf_cloud_shadow_device_status_update(&device_status);
	if (err) {
		mosh_error("Failed to update device shadow, error: %d", err);
	}
}

static K_WORK_DEFINE(shadow_update_work, shadow_update_work_fn);

static void nrf_cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	const int reconnection_delay = 10;

	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		mosh_warn("Add the device to nRF Cloud and reconnect");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_USER_ASSOCIATED");
		break;
	case NRF_CLOUD_EVT_READY:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_READY");
		mosh_print("Connection to nRF Cloud established");
		k_work_submit_to_queue(&mosh_common_work_q, &shadow_update_work);
		break;
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_RX_DATA_GENERAL");
		if (((char *)evt->data.ptr)[0] == '{') {
			/* Check if it's a MoSh command sent from the cloud */
			if (cloud_shell_parse_mosh_cmd(evt->data.ptr)) {
				k_work_submit_to_queue(&mosh_common_work_q,
						       &cloud_cmd_execute_work);
			}
		}
		break;
	case NRF_CLOUD_EVT_RX_DATA_LOCATION:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_RX_DATA_LOCATION");
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_RX_DATA_SHADOW");
		break;
	case NRF_CLOUD_EVT_PINGRESP:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_PINGRESP");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED, status: %d",
			   evt->status);
		mosh_print("Connection to nRF Cloud disconnected");
		if (!nfsm_get_disconnect_requested()) {
			mosh_print("Reconnecting in %d seconds...", reconnection_delay);
			k_work_reschedule_for_queue(&mosh_common_work_q, &cloud_reconnect_work,
						    K_SECONDS(reconnection_delay));
		}
		break;
	case NRF_CLOUD_EVT_FOTA_START:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_FOTA_START");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_FOTA_DONE");
		break;
	case NRF_CLOUD_EVT_FOTA_ERROR:
		mosh_error("nRF Cloud event: NRF_CLOUD_EVT_FOTA_ERROR");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		mosh_error("nRF Cloud event: NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR, status: %d",
			   evt->status);
		mosh_error("Connecting to nRF Cloud failed");
		break;
	case NRF_CLOUD_EVT_ERROR:
		mosh_print("nRF Cloud event: NRF_CLOUD_EVT_ERROR, status: %d", evt->status);
		break;
	default:
		mosh_error("Unknown nRF Cloud event type: %d", evt->type);
		break;
	}
}

static void cmd_cloud_connect(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	static bool initialized;

	if (!initialized) {
		struct nrf_cloud_init_param config = {
			.event_handler = nrf_cloud_event_handler,
		};

		err = nrf_cloud_init(&config);
		if (err == -EACCES) {
			mosh_print("nRF Cloud module already initialized");
		} else if (err) {
			mosh_error("nrf_cloud_init, error: %d", err);
			return;
		}

		initialized = true;
	}

	k_work_reschedule_for_queue(&mosh_common_work_q, &cloud_reconnect_work, K_NO_WAIT);

	mosh_print("Endpoint: %s", CONFIG_NRF_CLOUD_HOST_NAME);
}

static void cmd_cloud_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	/* Stop possibly pending reconnection attempt. */
	k_work_cancel_delayable(&cloud_reconnect_work);

	err = nrf_cloud_disconnect();
	if (err == -EACCES) {
		mosh_print("Not connected to nRF Cloud");
	} else if (err) {
		mosh_error("nrf_cloud_disconnect, error: %d", err);
	}
}

static int cmd_cloud(const struct shell *shell, size_t argc, char **argv)
{
	return cloud_shell_print_usage(shell, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_cloud,
	SHELL_CMD_ARG(connect, NULL, "Establish MQTT connection to nRF Cloud.", cmd_cloud_connect,
		      1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect from nRF Cloud.", cmd_cloud_disconnect, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cloud, &sub_cloud, "MQTT connection to nRF Cloud", cmd_cloud);
