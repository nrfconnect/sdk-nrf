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

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(remote, LOG_LEVEL_DBG);

static K_MUTEX_DEFINE(history_transfer_mtx);
static uint32_t history_transfer_id;
static log_rpc_history_handler_t history_handler;
static log_rpc_history_threshold_reached_handler_t history_threshold_reached_handler;

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

void log_rpc_set_stream_level(enum log_rpc_level level)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1 + sizeof(level));
	nrf_rpc_encode_uint(&ctx, level);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_SET_STREAM_LEVEL, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void log_rpc_set_history_level(enum log_rpc_level level)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1 + sizeof(level));
	nrf_rpc_encode_uint(&ctx, level);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_SET_HISTORY_LEVEL, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

int log_rpc_fetch_history(log_rpc_history_handler_t handler)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t transfer_id;

	k_mutex_lock(&history_transfer_mtx, K_FOREVER);
	transfer_id = ++history_transfer_id;
	history_handler = handler;
	k_mutex_unlock(&history_transfer_mtx);

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1 + sizeof(transfer_id));
	nrf_rpc_encode_uint(&ctx, transfer_id);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_FETCH_HISTORY, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	return 0;
}

void log_rpc_stop_fetch_history(bool pause)
{
	struct nrf_rpc_cbor_ctx ctx;

	k_mutex_lock(&history_transfer_mtx, K_FOREVER);
	history_handler = NULL;
	k_mutex_unlock(&history_transfer_mtx);

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, pause);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_STOP_FETCH_HISTORY, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void log_rpc_put_history_chunk_handler(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t transfer_id;
	enum log_rpc_level level;
	const char *message;
	size_t message_size;

	k_mutex_lock(&history_transfer_mtx, K_FOREVER);

	transfer_id = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decode_valid(ctx)) {
		goto out;
	}

	if (transfer_id != history_transfer_id) {
		/* Received outdated history chunk. */
		goto out;
	}

	if (nrf_rpc_decode_is_null(ctx)) {
		/* An empty history chunk indicates the end of history. */
		if (history_handler != NULL) {
			log_rpc_history_handler_t handler = history_handler;

			history_handler = NULL;
			handler(LOG_RPC_LEVEL_NONE, NULL, 0);
		}

		goto out;
	}

	while (nrf_rpc_decode_valid(ctx) && !nrf_rpc_decode_is_null(ctx)) {
		level = nrf_rpc_decode_uint(ctx);
		message = nrf_rpc_decode_buffer_ptr_and_size(ctx, &message_size);

		if (history_handler != NULL && message != NULL) {
			history_handler(level, message, message_size);
		}
	}

out:

	k_mutex_unlock(&history_transfer_mtx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, LOG_RPC_CMD_PUT_HISTORY_CHUNK,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(log_rpc_group, log_rpc_put_history_chunk_handler,
			 LOG_RPC_CMD_PUT_HISTORY_CHUNK, log_rpc_put_history_chunk_handler, NULL);

int log_rpc_get_crash_dump(size_t offset, uint8_t *buffer, size_t buffer_length)
{
	struct nrf_rpc_cbor_ctx ctx;
	const uint8_t *log_chunk;
	size_t log_chunk_size = 0;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 2 + sizeof(offset) + sizeof(buffer_length));
	nrf_rpc_encode_uint(&ctx, offset);
	nrf_rpc_encode_uint(&ctx, buffer_length);
	nrf_rpc_cbor_cmd_rsp_no_err(&log_rpc_group, LOG_RPC_CMD_GET_CRASH_DUMP, &ctx);

	/* Parse response */
	log_chunk = nrf_rpc_decode_buffer_ptr_and_size(&ctx, &log_chunk_size);

	if (log_chunk) {
		log_chunk_size = MIN(log_chunk_size, buffer_length);
		memcpy(buffer, log_chunk, log_chunk_size);
	}

	if (!nrf_rpc_decoding_done_and_check(&log_rpc_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &log_rpc_group,
			    LOG_RPC_CMD_GET_CRASH_DUMP, NRF_RPC_PACKET_TYPE_RSP);
	}

	return log_chunk_size;
}

int log_rpc_invalidate_crash_dump(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	int32_t rc;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_INVALIDATE_CRASH_DUMP, &ctx,
				nrf_rpc_rsp_decode_i32, &rc);

	return rc;
}

void log_rpc_echo(enum log_rpc_level level, const char *message)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t message_size = strlen(message);

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 4 + message_size);
	nrf_rpc_encode_uint(&ctx, level);
	nrf_rpc_encode_str(&ctx, message, message_size);
	nrf_rpc_cbor_cmd_rsp_no_err(&log_rpc_group, LOG_RPC_CMD_ECHO, &ctx);

	nrf_rpc_cbor_decoding_done(&log_rpc_group, &ctx);
}

static void log_rpc_history_threshold_reached_handler(const struct nrf_rpc_group *group,
						      struct nrf_rpc_cbor_ctx *ctx,
						      void *handler_data)
{
	nrf_rpc_cbor_decoding_done(&log_rpc_group, ctx);

	if (history_threshold_reached_handler) {
		history_threshold_reached_handler();
	}
}

NRF_RPC_CBOR_EVT_DECODER(log_rpc_group, log_rpc_history_threshold_reached_handler,
			 LOG_RPC_EVT_HISTORY_THRESHOLD_REACHED,
			 log_rpc_history_threshold_reached_handler, NULL);

uint8_t log_rpc_get_history_usage_threshold(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t threshold;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_GET_HISTORY_USAGE_THRESHOLD, &ctx,
				nrf_rpc_rsp_decode_u8, &threshold);

	return threshold;
}

size_t log_rpc_get_history_usage_current(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t usage_size;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_GET_HISTORY_USAGE_SIZE, &ctx,
				nrf_rpc_rsp_decode_u32, &usage_size);

	return (size_t)usage_size;
}

size_t log_rpc_get_history_usage_max(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t max_size;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_GET_HISTORY_MAX_SIZE, &ctx,
				nrf_rpc_rsp_decode_u32, &max_size);

	return (size_t)max_size;
}

void log_rpc_set_history_usage_threshold(log_rpc_history_threshold_reached_handler_t handler,
					 uint8_t threshold)
{
	struct nrf_rpc_cbor_ctx ctx;

	history_threshold_reached_handler = handler;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 2);
	nrf_rpc_encode_uint(&ctx, threshold);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_SET_HISTORY_USAGE_THRESHOLD, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void log_rpc_set_time(uint64_t now_us)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&log_rpc_group, ctx, 1 + sizeof(now_us));
	nrf_rpc_encode_uint64(&ctx, now_us);
	nrf_rpc_cbor_cmd_no_err(&log_rpc_group, LOG_RPC_CMD_SET_TIME, &ctx, nrf_rpc_rsp_decode_void,
				NULL);
}
