/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include "app_rpc.h"

#if IS_ENABLED(CONFIG_NRF_RPC_CBOR)
#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>
#endif

#define IS_RPC_CLIENT CONFIG_SOC_NRF5340_CPUNET
#define IS_RPC_SERVER CONFIG_SOC_NRF5340_CPUAPP

NRF_RPC_IPC_TRANSPORT(app_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "app_rpc_tr_ept");
NRF_RPC_GROUP_DEFINE(app_rpc_grp, "app_rpc_grp", &app_rpc_tr, NULL, NULL, NULL);

#define MAX_ENCODED_LEN 0

#define APP_RPC_CBOR_CMD_SYSTEM_REBOOT 0x00

#if IS_RPC_CLIENT
void app_system_reboot(void)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&app_rpc_grp, ctx, MAX_ENCODED_LEN);

	nrf_rpc_cbor_cmd_rsp_no_err(&app_rpc_grp, APP_RPC_CBOR_CMD_SYSTEM_REBOOT, &ctx);

	nrf_rpc_cbor_decoding_done(&app_rpc_grp, &ctx);
}
#endif /* IS_RPC_CLIENT */

#if IS_RPC_SERVER
static void remote_handler_app_system_reboot(const struct nrf_rpc_group *group,
			struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	struct nrf_rpc_cbor_ctx nctx;

	NRF_RPC_CBOR_ALLOC(group, nctx, MAX_ENCODED_LEN);
	nrf_rpc_cbor_rsp_no_err(group, &nctx);

	sys_reboot(SYS_REBOOT_COLD);
}

NRF_RPC_CBOR_CMD_DECODER(app_rpc_grp,
			remote_handler_app_system_reboot,
			APP_RPC_CBOR_CMD_SYSTEM_REBOOT,
			remote_handler_app_system_reboot,
			NULL);

#endif /* IS_RPC_SERVER */

#if (IS_RPC_CLIENT || IS_RPC_SERVER)
static void err_handler(const struct nrf_rpc_err_report *report)
{
	k_oops();
}

static int serialization_init(void)
{
	int err;

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -NRF_EINVAL;
	}

	return 0;
}

SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* IS_RPC_CLIENT || IS_RPC_SERVER */
