/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/udp.h>
#include <openthread/message.h>

#include <zephyr/net/openthread.h>

#define OT_MESSAGES_POOL 8

static otMessage *ot_message_registry[OT_MESSAGES_POOL];

ot_msg_key ot_reg_msg_alloc(otMessage *msg)
{
	for (ot_msg_key i = 0; i < OT_MESSAGES_POOL; i++) {
		if (ot_message_registry[i] == NULL) {
			ot_message_registry[i] = msg;
			return i + 1;
		}
	}
	return 0;
}

void ot_msg_free(ot_msg_key key)
{
	key--;
	if (key >= OT_MESSAGES_POOL) {
		return;
	}

	if (ot_message_registry[key] != NULL) {
		ot_message_registry[key] = NULL;
	}
}

otMessage *ot_msg_get(ot_msg_key key)
{
	key--;

	if (key < OT_MESSAGES_POOL) {
		return ot_message_registry[key];
	}

	return NULL;
}

static void ot_rpc_msg_free(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	ot_msg_key key = 0;
	bool decoding_ok;
	otMessage *message;

	decoding_ok = zcbor_uint_decode(ctx->zs, &key, sizeof(key));

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoding_ok) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_FREE);
		goto exit;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		otMessageFree(message);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	ot_msg_free(key);

exit:
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_msg_append(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	otError error = OT_ERROR_NONE;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	struct zcbor_string zst;
	uint32_t key;
	bool decoding_ok;
	otMessage *message;

	decoding_ok = zcbor_uint_decode(ctx->zs, &key, sizeof(uint32_t));

	if (!decoding_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	decoding_ok = zcbor_bstr_decode(ctx->zs, &zst);

	if (!decoding_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		error = otMessageAppend(ot_msg_get(key), zst.value, zst.len);
		openthread_api_mutex_unlock(openthread_get_default_context());
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

exit:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(otError) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(otError));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_udp_new(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	ot_msg_key key = 0;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otMessageSettings settings;
	otMessageSettings *pSettings;

	pSettings = nrf_rpc_decode_buffer(ctx, &settings, sizeof(otMessageSettings));

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	key = ot_reg_msg_alloc(otUdpNewMessage(openthread_get_default_instance(), pSettings));
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (ot_msg_get(key) == NULL) {
		key = 0;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(key) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &key, sizeof(key));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_length(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	ot_msg_key key;
	bool decoding_ok;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint16_t length = 0;
	otMessage *message;

	decoding_ok = zcbor_uint_decode(ctx->zs, &key, sizeof(key));

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoding_ok) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		goto exit;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		length = otMessageGetLength(message);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

exit:

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(length) + 2);
	zcbor_uint_encode(rsp_ctx.zs, &length, sizeof(length));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_get_offset(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	ot_msg_key key;
	bool decoding_ok;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint16_t offset = 0;
	otMessage *message;

	decoding_ok = zcbor_uint_decode(ctx->zs, &key, sizeof(key));

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoding_ok) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		goto exit;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		offset = otMessageGetOffset(message);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

exit:

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(offset) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &offset, sizeof(offset));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_read(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	uint16_t offset;
	uint16_t length;
	ot_msg_key key;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const uint16_t chunk_size = 64;
	uint8_t buf[chunk_size];
	uint16_t read = 0;
	otMessage *message;

	if (!zcbor_uint_decode(ctx->zs, &key, sizeof(key))) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		goto exit;
	}

	if (!zcbor_uint_decode(ctx->zs, &offset, sizeof(offset))) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		goto exit;
	}

	if (!zcbor_uint_decode(ctx->zs, &length, sizeof(length))) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, length + 2);

	message = ot_msg_get(key);

	if (message == NULL) {
		zcbor_nil_put(rsp_ctx.zs, NULL);
		goto exit;
	}

	if (!zcbor_bstr_start_encode(rsp_ctx.zs)) {
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());

	do {
		read = otMessageRead(message, offset, buf,
				     (chunk_size < length) ? chunk_size : length);
		memcpy(rsp_ctx.zs[0].payload_mut, buf, read);
		rsp_ctx.zs->payload_mut += read;
		length -= read;
		offset += read;
	} while (read > 0 && length > 0);

	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_bstr_end_encode(rsp_ctx.zs, NULL)) {
		goto exit;
	}

exit:
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_length, OT_RPC_CMD_MESSAGE_GET_LENGTH,
			 ot_rpc_msg_length, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_get_offset, OT_RPC_CMD_MESSAGE_GET_OFFSET,
			 ot_rpc_get_offset, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_read, OT_RPC_CMD_MESSAGE_READ, ot_rpc_msg_read, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_free, OT_RPC_CMD_MESSAGE_FREE, ot_rpc_msg_free, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_udp_new, OT_RPC_CMD_UDP_NEW_MESSAGE,
			 ot_rpc_msg_udp_new, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_append, OT_RPC_CMD_MESSAGE_APPEND, ot_rpc_msg_append,
			 NULL);
