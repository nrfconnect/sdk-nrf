/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements Remote Procedure Calls (RPC) for Software Coexistence implementation.
 *
 */

#include <mpsl/mpsl_cx_software.h>

#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_uart.h>

#include <zephyr/device.h>

#ifdef CONFIG_NRF_RPC_IPC_SERVICE
NRF_RPC_IPC_TRANSPORT(mpsl_cx_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "mpsl_cx_rpc_ept");
#elif defined(CONFIG_NRF_RPC_UART_TRANSPORT)
#define mpsl_cx_rpc_tr NRF_RPC_UART_TRANSPORT(DT_CHOSEN(nordic_rpc_uart))
#endif
NRF_RPC_GROUP_DEFINE(mpsl_cx_rpc_group, "mpsl_cx", &mpsl_cx_rpc_tr, NULL, NULL, NULL);

enum mpsl_cx_rpc_cmd {
	MPSL_CX_RPC_CMD_SET_GRANTED_OPS = 0,
};

#ifdef CONFIG_MPSL_CX_SOFTWARE_RPC_CLIENT
int mpsl_cx_software_set_granted_ops(mpsl_cx_op_map_t ops, k_timeout_t timeout)
{
	struct nrf_rpc_cbor_ctx ctx;
	int64_t timeout_ms;
	int32_t rc;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = -1;
	} else {
		timeout_ms = k_ticks_to_ms_ceil64(timeout.ticks);
	}

	NRF_RPC_CBOR_ALLOC(&mpsl_cx_rpc_group, ctx, 11);

	if (!zcbor_uint_encode(ctx.zs, &ops, sizeof(ops)) || !zcbor_int64_put(ctx.zs, timeout_ms)) {
		return -ENOBUFS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&mpsl_cx_rpc_group, MPSL_CX_RPC_CMD_SET_GRANTED_OPS, &ctx);

	if (!zcbor_uint32_decode(ctx.zs, &rc)) {
		rc = -EINVAL;
	}

	nrf_rpc_cbor_decoding_done(&mpsl_cx_rpc_group, &ctx);

	return rc;
}
#endif /* CONFIG_MPSL_CX_SOFTWARE_RPC_CLIENT */

#ifdef CONFIG_MPSL_CX_SOFTWARE_RPC_SERVER
void mpsl_cx_software_set_granted_ops_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	mpsl_cx_op_map_t ops;
	int64_t timeout_ms;
	bool decoded_ok;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	int32_t rc;

	decoded_ok = zcbor_uint_decode(ctx->zs, &ops, sizeof(ops));
	decoded_ok = decoded_ok && zcbor_int64_decode(ctx->zs, &timeout_ms);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		rc = -EINVAL;
		goto out;
	}

	rc = mpsl_cx_software_set_granted_ops(ops,
					      timeout_ms >= 0 ? K_MSEC(timeout_ms) : K_FOREVER);

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint32_put(rsp_ctx.zs, rc);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(mpsl_cx_rpc_group, mpsl_cx_software_set_granted_ops_handler,
			 MPSL_CX_RPC_CMD_SET_GRANTED_OPS, mpsl_cx_software_set_granted_ops_handler,
			 NULL);
#endif /* CONFIG_MPSL_CX_SOFTWARE_RPC_SERVER */
