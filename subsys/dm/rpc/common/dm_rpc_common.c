/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

#include "dm_rpc_common.h"

LOG_MODULE_DECLARE(nrf_dm, CONFIG_DM_MODULE_LOG_LEVEL);

NRF_RPC_IPC_TRANSPORT(dm_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "dm_rpc_ept");
NRF_RPC_GROUP_DEFINE(dm_rpc_grp, "dm_rpc_grp", &dm_rpc_tr, NULL, NULL, NULL);

#if CONFIG_DM_MODULE_RPC_INITIALIZE_NRF_RPC
static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
		report->code);
	k_oops();
}

static int serialization_init(void)
{

	int err;

	LOG_DBG("Init begin");
	err = nrf_rpc_init(err_handler);
	if (err) {
		return -EINVAL;
	}

	LOG_DBG("Init done\n");

	return 0;
}

SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_DM_MODULE_RPC_INITIALIZE_NRF_RPC */
