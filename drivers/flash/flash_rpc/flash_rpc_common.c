/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

LOG_MODULE_REGISTER(FLASH_RPC, CONFIG_FLASH_RPC_LOG_LEVEL);

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
	       report->code);
	k_oops();
}

static int serialization_init(void)
{
	int err;

	err = nrf_rpc_init(err_handler);
	if (err) {
		LOG_ERR("Initializing nRF RPC failed: %d", err);
		return -EINVAL;
	}

	return 0;
}

SYS_INIT(serialization_init, POST_KERNEL, CONFIG_FLASH_RPC_SYS_INIT_PRIORITY);
