/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include "mosh_print.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_REST));

extern struct k_work_q mosh_common_work_q;

static int cmd_cloud_rest_shadow_device_status_update(const struct shell *shell, size_t argc,
						      char **argv)
{
	int err;
	struct nrf_cloud_modem_info modem_info = {
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = NRF_CLOUD_INFO_SET,
		.mpi = NULL
	};
	struct nrf_cloud_device_status device_status = {
		.modem = &modem_info,
		/* Deprecated: The service info "ui" section is no longer used by nRF Cloud */
		.svc = NULL
	};
	char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
#define REST_RX_BUF_SZ 300 /* No payload in response, "just" headers */
	static char rx_buf[REST_RX_BUF_SZ];
	struct nrf_cloud_rest_context rest_ctx = { .connect_socket = -1,
						   .keep_alive = false,
						   .rx_buf = rx_buf,
						   .rx_buf_len = sizeof(rx_buf),
						   .timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
						   .fragment_size = 0 };

	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		mosh_error("Failed to get device ID, error: %d", err);
		return err;
	}

	err = nrf_cloud_rest_shadow_device_status_update(&rest_ctx, device_id, &device_status);
	if (err) {
		mosh_error("REST: shadow update failed: %d", err);
		return err;
	}
	mosh_print("Device status updated to cloud");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cloud_rest,
			       SHELL_CMD(shadow_update, NULL,
					 "Send device capabilities to nRF Cloud.",
					 cmd_cloud_rest_shadow_device_status_update),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(
	cloud_rest,
	&sub_cloud_rest,
	"Send nRF Cloud command over REST",
	mosh_print_help_shell);
