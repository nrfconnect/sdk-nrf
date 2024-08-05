/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include <nrf_rpc.h>

LOG_MODULE_REGISTER(nrf_ps_client, CONFIG_NRF_PS_CLIENT_LOG_LEVEL);

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details", report->code);
}

int main(void)
{
	int ret;

	LOG_INF("Initializing RPC client");

	ret = nrf_rpc_init(err_handler);

	if (ret != 0) {
		LOG_ERR("RPC init failed");
	}

	LOG_INF("RPC client ready");

	return 0;
}
