/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT_RPC_CMD_PING server decoder (UART / RPC link test).
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"
#include "dect_rpc_server_internal.h"

#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

static void dect_rpc_cmd_ping(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	(void)nrf_rpc_decode_skip(ctx);
	nrf_rpc_decoding_done(group, ctx->in_packet);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 4);
	nrf_rpc_encode_uint(&rsp_ctx, 0U);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_ping, DECT_RPC_CMD_PING,
			 dect_rpc_cmd_ping, NULL);
