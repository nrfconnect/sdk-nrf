/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <net/nrf_cloud_coap.h>
#include <net/nrf_cloud_alert.h>

#include <zephyr/shell/shell.h>

#include "desh_print.h"

#if defined(CONFIG_SAMPLE_DESH_NATIVE_TLS)
#include "desh_native_tls.h"
#endif

extern struct k_work_q desh_common_work_q;

BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_COAP));
BUILD_ASSERT(!IS_ENABLED(CONFIG_NRF_CLOUD_MQTT));

static bool connected;

static void coap_connect_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	if (connected) {
		desh_error("Already connected to nRF Cloud");
		return;
	}

	err = nrf_cloud_coap_connect(desh_print_version_str_get());
	if (err) {
		desh_error("Connecting to nRF Cloud failed, error: %d", err);
		return;
	}

	connected = true;
}

K_WORK_DEFINE(coap_connect_work, coap_connect_work_fn);

static void cmd_cloud_coap_connect(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;
	static bool initialized;

	if (!initialized) {
		err = nrf_cloud_coap_init();
		if (err) {
			desh_error("Initializing nRF Cloud CoAP library failed, error: %d", err);
			return;
		}

		initialized = true;
	}

	k_work_submit_to_queue(&desh_common_work_q, &coap_connect_work);
}

static void cmd_cloud_coap_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;

	if (!connected) {
		desh_error("Not connected to nRF Cloud");
		return;
	}

	err = nrf_cloud_coap_disconnect();
	if (err) {
		desh_error("Disconnecting from nRF Cloud failed, error: %d", err);
	}

	connected = false;
}

static void cmd_cloud_coap_alert_tx(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (argc < 2) {
		desh_error("Alert data not provided");
		return;
	}
	int err;
	char *alert_data = argv[1];

	err = nrf_cloud_alert_send(ALERT_TYPE_MSG, 0, alert_data);
	if (err) {
		desh_error("Sending to nRF Cloud failed, error: %d", err);
	} else {
		desh_print("Alert data sent to nRF Cloud");
	}
}

static void cmd_cloud_coap_raw_data_tx(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;
	char *raw_data = argv[1];
	size_t raw_data_len = strlen(raw_data);

	err = nrf_cloud_coap_bytes_send(raw_data, raw_data_len + 1, true);
	if (err) {
		desh_error("Sending raw data to nRF Cloud failed, error: %d", err);
	} else {
		desh_print("Raw data sent to nRF Cloud");
	}
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_cloud,
	SHELL_CMD_ARG(connect, NULL, "Establish CoAP connection to nRF Cloud.",
		      cmd_cloud_coap_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect from nRF Cloud.", cmd_cloud_coap_disconnect, 1,
		      0),
	SHELL_CMD_ARG(raw_data_tx, NULL, " Send raw bytes to nRF Cloud on the /msg/d2c/raw topic.",
		      cmd_cloud_coap_raw_data_tx, 2, 0),
	SHELL_CMD_ARG(alert_tx, NULL, " Send given alert message to nRF Cloud.",
		      cmd_cloud_coap_alert_tx, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cloud, &sub_cloud, "CoAP connection to nRF Cloud", desh_print_help_shell);
