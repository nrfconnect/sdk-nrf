/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define NRF_RPC_LOG_MODULE NRF_RPC_INIT
#include <nrf_rpc_log.h>

#include <zephyr/init.h>

#include <nrf_rpc.h>

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details",
		report->code);
	k_oops();
}

static int nrf_rpc_sys_init(void)
{
	int err;

	LOG_DBG("RPC init begin");

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -EINVAL;
	}

	LOG_DBG("RPC init done");

	return 0;
}

SYS_INIT(nrf_rpc_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
