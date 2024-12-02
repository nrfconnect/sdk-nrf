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
	const char *message;
	size_t message_size;

	level = nrf_rpc_decode_uint(ctx);
	message = nrf_rpc_decode_buffer_ptr_and_size(ctx, &message_size);

	if (message) {
		switch (level) {
		case LOG_RPC_LEVEL_ERR:
			LOG_ERR("%.*s", message_size, message);
			break;
		case LOG_RPC_LEVEL_WRN:
			LOG_WRN("%.*s", message_size, message);
			break;
		case LOG_RPC_LEVEL_INF:
			LOG_INF("%.*s", message_size, message);
			break;
		case LOG_RPC_LEVEL_DBG:
			LOG_DBG("%.*s", message_size, message);
			break;
		default:
			break;
		}
	}

	if (!nrf_rpc_decoding_done_and_check(&log_rpc_group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &log_rpc_group, LOG_RPC_EVT_MSG,
			    NRF_RPC_PACKET_TYPE_EVT);
	}
}

NRF_RPC_CBOR_EVT_DECODER(log_rpc_group, log_rpc_msg_handler, LOG_RPC_EVT_MSG, log_rpc_msg_handler,
			 NULL);

int log_rpc_set_stream_level(enum log_rpc_level level)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1 + sizeof(level));
	nrf_rpc_encode_uint(&ctx, level);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_SET_STREAM_LEVEL, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	return 0;
}

int log_rpc_get_crash_log(size_t offset, char *buffer, size_t buffer_length)
{
	struct nrf_rpc_cbor_ctx ctx;
	const uint8_t *log_chunk;
	size_t log_chunk_size = 0;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 2 + sizeof(offset) + sizeof(buffer_length));
	nrf_rpc_encode_uint(&ctx, offset);
	nrf_rpc_encode_uint(&ctx, buffer_length);
	nrf_rpc_cbor_cmd_rsp_no_err(&log_rpc_group, LOG_RPC_CMD_GET_CRASH_LOG, &ctx);

	/* Parse response */
	log_chunk = nrf_rpc_decode_buffer_ptr_and_size(&ctx, &log_chunk_size);

	if (log_chunk) {
		log_chunk_size = MIN(log_chunk_size, buffer_length);
		memcpy(buffer, log_chunk, log_chunk_size);
	}

	if (!nrf_rpc_decoding_done_and_check(&log_rpc_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &log_rpc_group,
			    LOG_RPC_CMD_GET_CRASH_LOG, NRF_RPC_PACKET_TYPE_RSP);
	}

	return log_chunk_size;
}
