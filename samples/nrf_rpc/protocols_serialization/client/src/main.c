/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_shell.h"

#include <zephyr/logging/log.h>

#include <nrf_rpc.h>

LOG_MODULE_REGISTER(nrf_ps_client, CONFIG_NRF_PS_CLIENT_LOG_LEVEL);
NRF_RPC_GROUP_DECLARE(ot_group);
NRF_RPC_GROUP_DECLARE(bt_rpc_grp);

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details", report->code);
}

static void bound_handler(const struct nrf_rpc_group *group)
{
#ifdef CONFIG_OPENTHREAD_RPC_CLIENT
	static bool ot_group_initialized;

	if (group == &ot_group) {
		if (ot_group_initialized) {
			LOG_WRN("OT RPC peer reset detected");
			ot_shell_server_restarted();
			/* The code to restore the state of OT on the server can be added here */
		} else {
			ot_group_initialized = true;
		}
	}
#endif
#ifdef CONFIG_BT_RPC_CLIENT
	static bool bt_group_initialized;

	if (group == &bt_rpc_grp) {
		if (bt_group_initialized) {
			LOG_WRN("BT RPC peer reset detected");
			/* The code to restore the state of BT on the server can be added here */
		} else {
			bt_group_initialized = true;
		}
	}
#endif
}

int main(void)
{
	int ret;

	LOG_INF("Initializing RPC client");

	nrf_rpc_set_bound_handler(bound_handler);

	ret = nrf_rpc_init(err_handler);

	if (ret != 0) {
		LOG_ERR("RPC init failed");
	}

	LOG_INF("RPC client ready");

	return 0;
}
