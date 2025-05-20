/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <net/nrf_cloud_coap.h>
#include <zephyr/shell/shell.h>
#include "mosh_print.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_COAP));

BUILD_ASSERT(!IS_ENABLED(CONFIG_MOSH_CLOUD_LWM2M));

extern struct k_work_q mosh_common_work_q;

static bool connected;

static void coap_connect_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	if (connected) {
		mosh_error("Already connected to nRF Cloud");
		return;
	}

	err = nrf_cloud_coap_connect(NULL);
	if (err) {
		mosh_error("Connecting to nRF Cloud failed, error: %d", err);
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
			mosh_error("Initializing nRF Cloud CoAP library failed, error: %d", err);
			return;
		}

		initialized = true;
	}

	k_work_submit_to_queue(&mosh_common_work_q, &coap_connect_work);
}

static void cmd_cloud_coap_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;

	if (!connected) {
		mosh_error("Not connected to nRF Cloud");
		return;
	}

	err = nrf_cloud_coap_disconnect();
	if (err) {
		mosh_error("Disconnecting from nRF Cloud failed, error: %d", err);
	}

	connected = false;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_cloud,
	SHELL_CMD_ARG(connect, NULL, "Establish CoAP connection to nRF Cloud.",
		      cmd_cloud_coap_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect from nRF Cloud.",
		      cmd_cloud_coap_disconnect, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cloud, &sub_cloud, "CoAP connection to nRF Cloud", mosh_print_help_shell);
