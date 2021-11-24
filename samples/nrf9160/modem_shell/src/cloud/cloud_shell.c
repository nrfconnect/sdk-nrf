/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr.h>
#include <net/nrf_cloud.h>
#include <nrf_cloud_fsm.h>
#include <shell/shell.h>
#include "mosh_print.h"

#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_agps.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif

BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) &&
	IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD));

static struct k_work_delayable cloud_reconnect_work;
#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work notify_pgps_work;
#endif

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
	int err = nrf_cloud_connect(NULL);

	if (err == NRF_CLOUD_CONNECT_RES_SUCCESS) {
		mosh_print("Connection to nRF Cloud established");
	} else if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		mosh_print("nRF Cloud connection already established");
	} else {
		mosh_error("nrf_cloud_connect, error: %d", err);
	}
}

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void notify_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		mosh_error("Error requesting notification of prediction availability: %d", err);
	}
}
#endif /* defined(CONFIG_NRF_CLOUD_PGPS) */

static void nrf_cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	int err = 0;
	const int reconnection_delay = 10;

	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		mosh_print("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		mosh_print("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_READY:
		mosh_print("NRF_CLOUD_EVT_READY");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		mosh_print("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		if (!nfsm_get_disconnect_requested()) {
			mosh_print("Reconnecting in %d seconds...", reconnection_delay);
			k_work_reschedule(&cloud_reconnect_work, K_SECONDS(reconnection_delay));
		}
		break;
	case NRF_CLOUD_EVT_ERROR:
		mosh_print("NRF_CLOUD_EVT_ERROR");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		mosh_print("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		mosh_print("NRF_CLOUD_EVT_FOTA_DONE");
		break;
	case NRF_CLOUD_EVT_RX_DATA:
		mosh_print("NRF_CLOUD_EVT_RX_DATA");
		if (((char *)evt->data.ptr)[0] == '{') {
			/* Not A-GPS data */
			break;
		}
#if defined(CONFIG_NRF_CLOUD_AGPS)
		err = nrf_cloud_agps_process((char *)evt->data.ptr, evt->data.len);
		if (!err) {
			mosh_print("A-GPS data processed");
#if defined(CONFIG_NRF_CLOUD_PGPS)
			/* call us back when prediction is ready */
			k_work_submit(&notify_pgps_work);
#endif
			/* data was valid; no need to pass to other handlers */
			break;
		}
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
		err = nrf_cloud_pgps_process(evt->data.ptr, evt->data.len);
		if (err) {
			mosh_error("Error processing P-GPS packet: %d", err);
		}
#else
		if (err) {
			mosh_print("Unable to process A-GPS data, error: %d", err);
		}
#endif
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		mosh_print("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		mosh_warn("Add the device to nRF Cloud and reconnect");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		mosh_print("NRF_CLOUD_EVT_USER_ASSOCIATED");
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
#if defined(CONFIG_NRF_CLOUD_PGPS)
		k_work_init(&notify_pgps_work, notify_pgps);
#endif
		k_work_init_delayable(&cloud_reconnect_work, cloud_reconnect_work_fn);
	}

	k_work_reschedule(&cloud_reconnect_work, K_NO_WAIT);

	mosh_print("Endpoint: %s", CONFIG_NRF_CLOUD_HOST_NAME);
}

static void cmd_cloud_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	int err = nrf_cloud_disconnect();

	if (err == -EACCES) {
		mosh_print("Not connected to nRF Cloud");
	} else if (err) {
		mosh_error("nrf_cloud_disconnect, error: %d", err);
		return;
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
