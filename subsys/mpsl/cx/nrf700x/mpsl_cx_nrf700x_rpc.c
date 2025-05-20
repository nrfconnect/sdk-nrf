/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements Remote Procedure Calls (RPC) required to enable and disable
 *   the nRF700x Coexistence Interface from the nRF5340 application core.
 *
 */

#include <mpsl/mpsl_cx_nrf700x.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mpsl_cx_nrf700x_rpc, LOG_LEVEL_ERR);

#if defined(CONFIG_NRF_RPC_CBOR)
#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

NRF_RPC_IPC_TRANSPORT(
	mpsl_cx_nrf700x_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "mpsl_cx_nrf700x_rpc_tr_ept");
NRF_RPC_GROUP_DEFINE(
	mpsl_cx_nrf700x_rpc_grp, "mpsl_cx_nrf700x", &mpsl_cx_nrf700x_rpc_tr, NULL, NULL, NULL);

#define MPSL_CX_NRF700X_SET_ENABLED_CBOR_CMD  0x01
#endif

#define IS_RPC_CLIENT CONFIG_SOC_NRF5340_CPUAPP
#define IS_RPC_SERVER CONFIG_SOC_NRF5340_CPUNET

#if IS_RPC_CLIENT
void mpsl_cx_nrf700x_set_enabled(bool enable)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&mpsl_cx_nrf700x_rpc_grp, ctx, 1);

	zcbor_bool_put(ctx.zs, enable);

	nrf_rpc_cbor_cmd_rsp_no_err(
		&mpsl_cx_nrf700x_rpc_grp, MPSL_CX_NRF700X_SET_ENABLED_CBOR_CMD, &ctx);

	nrf_rpc_cbor_decoding_done(&mpsl_cx_nrf700x_rpc_grp, &ctx);
}
#endif /* IS_RPC_CLIENT */

#if IS_RPC_SERVER
void remote_handler_mpsl_cx_nrf700x_set_enabled(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx,
						void *handler_data)
{
	bool enable;

	if (!zcbor_bool_decode(ctx->zs, &enable)) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		return;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	mpsl_cx_nrf700x_set_enabled(enable);

	struct nrf_rpc_cbor_ctx nctx;

	NRF_RPC_CBOR_ALLOC(group, nctx, 0);

	nrf_rpc_cbor_rsp_no_err(group, &nctx);
}

NRF_RPC_CBOR_CMD_DECODER(mpsl_cx_nrf700x_rpc_grp,
			 remote_handler_mpsl_cx_nrf700x_set_enabled,
			 MPSL_CX_NRF700X_SET_ENABLED_CBOR_CMD,
			 remote_handler_mpsl_cx_nrf700x_set_enabled,
			 NULL);
#endif /* IS_RPC_SERVER */

#if IS_RPC_CLIENT || IS_RPC_SERVER

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
		report->code);
	k_oops();
}

static int serialization_init(void)
{

	return nrf_rpc_init(err_handler);
}

SYS_INIT(serialization_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* IS_RPC_CLIENT && IS_RPC_SERVER */
