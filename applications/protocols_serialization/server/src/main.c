/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include <nrf_rpc.h>

LOG_MODULE_REGISTER(nrf_rpc_remote, CONFIG_NRF_RPC_REMOTE_LOG_LEVEL);

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details", report->code);
	//k_oops();
}

int main(void)
{
	int ret;

	LOG_INF("Initializing RPC server");

	ret = nrf_rpc_init(err_handler);

	if (ret != 0) {
		LOG_ERR("RPC init failed");
	}

	LOG_INF("RPC server ready");

	return 0;
}
