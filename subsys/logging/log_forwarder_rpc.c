/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements Remote Procedure Calls (RPC) for logging subsystem.
 *
 */

#include "log_rpc_group.h"

#include <logging/log_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(remote);

static void log_rpc_msg_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	enum log_rpc_level level;
	struct zcbor_string message;
	bool decoded_ok;

	decoded_ok = zcbor_uint_decode(ctx->zs, &level, sizeof(level));
	decoded_ok = decoded_ok && zcbor_bstr_decode(ctx->zs, &message);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		return;
	}

	switch (level) {
	case LOG_RPC_LEVEL_ERR:
		LOG_ERR("%.*s", message.len, message.value);
		break;
	case LOG_RPC_LEVEL_WRN:
		LOG_WRN("%.*s", message.len, message.value);
		break;
	case LOG_RPC_LEVEL_INF:
		LOG_INF("%.*s", message.len, message.value);
		break;
	case LOG_RPC_LEVEL_DBG:
		LOG_DBG("%.*s", message.len, message.value);
		break;
	default:
		break;
	}
}

NRF_RPC_CBOR_EVT_DECODER(log_rpc_group, log_rpc_msg_handler, LOG_RPC_EVT_MSG, log_rpc_msg_handler,
			 NULL);

int log_rpc_set_stream_level(enum log_rpc_level level)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1 + sizeof(level));
	nrf_rpc_encode_uint(&ctx, level);
	nrf_rpc_cbor_cmd_rsp_no_err(&log_rpc_group, LOG_RPC_CMD_SET_STREAM_LEVEL, &ctx);

	nrf_rpc_cbor_decoding_done(&log_rpc_group, &ctx);

	return 0;
}

int log_rpc_get_crash_log(size_t offset, char *buffer, size_t buffer_length)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct zcbor_string log_chunk;
	bool decoded_ok;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 10);

	if (!zcbor_uint32_put(ctx.zs, offset) || !zcbor_uint32_put(ctx.zs, buffer_length)) {
		NRF_RPC_CBOR_DISCARD(&log_rpc_group, ctx);
		return -ENOBUFS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&log_rpc_group, LOG_RPC_CMD_GET_CRASH_LOG, &ctx);

	/* Parse response */
	decoded_ok = zcbor_bstr_decode(ctx.zs, &log_chunk);
	nrf_rpc_cbor_decoding_done(&log_rpc_group, &ctx);

	if (!decoded_ok) {
		return -EINVAL;
	}

	log_chunk.len = MIN(log_chunk.len, buffer_length);
	memcpy(buffer, log_chunk.value, log_chunk.len);

	return log_chunk.len;
}
