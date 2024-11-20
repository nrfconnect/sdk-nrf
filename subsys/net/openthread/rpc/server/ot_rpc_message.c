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

#define OT_MESSAGES_POOL CONFIG_OPENTHREAD_RPC_MESSAGE_POOL

static otMessage *ot_message_registry[OT_MESSAGES_POOL];

ot_msg_key ot_reg_msg_alloc(otMessage *msg)
{
	if (msg == NULL) {
		return 0;
	}

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
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_FREE);
		return;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		otMessageFree(message);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	ot_msg_free(key);

	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_msg_append(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	otError error = OT_ERROR_INVALID_ARGS;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint32_t key;
	const void *data;
	size_t size = 0;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);
	data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);

	if (data && size && nrf_rpc_decode_valid(ctx)) {
		message = ot_msg_get(key);

		if (message != NULL) {
			openthread_api_mutex_lock(openthread_get_default_context());
			error = otMessageAppend(message, data, size);
			openthread_api_mutex_unlock(openthread_get_default_context());
		}
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(otError) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_udp_new(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	ot_msg_key key;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otMessageSettings settings;
	otMessageSettings *p_settings = &settings;

	p_settings = ot_rpc_decode_message_settings(ctx, &settings);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_NEW_MESSAGE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	key = ot_reg_msg_alloc(otUdpNewMessage(openthread_get_default_instance(), p_settings));
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (ot_msg_get(key) == NULL) {
		key = 0;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(key) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, key);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_length(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	ot_msg_key key;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint16_t length = 0;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_GET_LENGTH);
		return;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		length = otMessageGetLength(message);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(length) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, length);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_get_offset(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	ot_msg_key key;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint16_t offset = 0;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_GET_OFFSET);
		return;
	}

	message = ot_msg_get(key);

	if (message != NULL) {
		openthread_api_mutex_lock(openthread_get_default_context());
		offset = otMessageGetOffset(message);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(offset) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, offset);
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

	key = nrf_rpc_decode_uint(ctx);
	offset = nrf_rpc_decode_uint(ctx);
	length = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_READ);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, length + 2);

	message = ot_msg_get(key);

	if (message == NULL) {
		nrf_rpc_encode_null(&rsp_ctx);
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
