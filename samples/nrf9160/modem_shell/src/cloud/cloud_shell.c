/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr.h>
#include <net/nrf_cloud.h>
#include <shell/shell.h>
#include "mosh_print.h"

#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_agps.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
//#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#endif

BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD) &&
	IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) &&
	IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD));

static const char cloud_shell_cmd_usage_str[] =
	"MQTT connection to nRFCloud\n"
	"Usage: cloud <subcommand>\n"
	"\n"
	"Subcommands:\n"
	"  connect:      Establish MQTT connection to nRFCloud.\n"
	"  disconnect:   Disconnect from nRFCloud.\n";

static void cloud_shell_print_usage(const struct shell *shell)
{
	mosh_print_no_format(cloud_shell_cmd_usage_str);
}

#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
static int send_service_info(void)
{
	/* Update nRF Cloud with GPS service info signifying that the device
	 * has GPS capabilities. Add other services as needed.
	 */
	int err;
	struct nrf_cloud_svc_info_fota fota_info = {
		.application = false,
		.bootloader = false,
		.modem = false
	};
	struct nrf_cloud_svc_info_ui ui_info = {
		.gps = true,
		.humidity = false,
		.rsrp = false,
		.temperature = false,
		.button = false
	};
	struct nrf_cloud_svc_info service_info = {
		.fota = &fota_info,
		.ui = &ui_info
	};
	struct nrf_cloud_device_status device_status = {
		.modem = NULL,
		.svc = &service_info

	};

	err = nrf_cloud_shadow_device_status_update(&device_status);
	if (err) {
		mosh_error("nrf_cloud_shadow_device_status_update, error: %d", err);
		return err;
	}

	mosh_print("nRF Cloud service info sent");

	return 0;
}
#endif /* defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS) */

static void nrf_cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		mosh_print("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		mosh_print("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_READY:
		mosh_print("NRF_CLOUD_EVT_READY");
#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
		int err = send_service_info();

		if (err) {
			mosh_error("Failed to send nRF Cloud service information");
		}
#endif
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		mosh_print("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
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
			// Not A-GPS data
			break;
		}
#if defined(CONFIG_NRF_CLOUD_AGPS)
		/* Put the following line to a work queue unless we can get rid of handling the
		 * A-GPS data in the application altogether
		 */
		err = nrf_cloud_agps_process((char *)evt->data.ptr, evt->data.len);
		if (!err) {
			mosh_print("A-GPS data processed");
#if defined(CONFIG_NRF_CLOUD_PGPS)
			/* call us back when prediction is ready */
			nrf_cloud_pgps_notify_prediction();
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
		mosh_warn("Add the device to nRF Cloud and wait for it to reconnect");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		mosh_print("NRF_CLOUD_EVT_USER_ASSOCIATED");
		break;
	default:
		mosh_error("Unknown nRF Cloud event type: %d", evt->type);
		break;
	}
}

static void cloud_connect(void)
{
	int err;

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

	err = nrf_cloud_connect(NULL);
	if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		mosh_print("nRF Cloud connection already established");
	} else if (err) {
		mosh_error("nrf_cloud_connect, error: %d", err);
	} else {
		mosh_print("********************************************");
		mosh_print(" Connection to nRF Cloud established");
		mosh_print(" Endpoint:    %s", CONFIG_NRF_CLOUD_HOST_NAME);
		mosh_print("********************************************");
	}

}

static void cloud_disconnect(void)
{
	int err = nrf_cloud_disconnect();

	if (err == -EACCES) {
		mosh_print("Not connected to nRF Cloud");
	} else if (err) {
		mosh_error("nrf_cloud_disconnect, error: %d", err);
		return;
	}
}

int cloud_shell(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	/* command = argv[0] = "cloud" */
	/* sub-command = argv[1]       */
	if (argc < 2) {
		goto show_usage;
	} else if (strcmp(argv[1], "connect") == 0) {
		cloud_connect();
	} else if (strcmp(argv[1], "disconnect") == 0) {
		cloud_disconnect();
	} else if (strcmp(argv[1], "?") == 0) {
		goto show_usage;
	} else {
		mosh_error("Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	return ret;

show_usage:
	cloud_shell_print_usage(shell);
	return ret;
}
