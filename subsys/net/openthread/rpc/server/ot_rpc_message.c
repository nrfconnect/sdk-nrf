/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_callback.h>
#include <ot_rpc_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_lock.h>
#include <ot_rpc_types.h>

#include <nrf_rpc_cbor.h>
#include "ot_rpc_resource.h"

#include <openthread/udp.h>
#include <openthread/message.h>

#include <zephyr/net/openthread.h>

OT_RPC_RESOURCE_TABLE_REGISTER(msg, otMessage, CONFIG_OPENTHREAD_RPC_MESSAGE_POOL);

static void ot_rpc_msg_free(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	ot_rpc_res_tab_key key;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_FREE);
		return;
	}

	ot_rpc_mutex_lock();
	message = ot_res_tab_msg_get(key);

	if (message != NULL) {
		otMessageFree(message);
		ot_res_tab_msg_free(key);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_void(group);
}

static void ot_rpc_msg_append(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	otError error = OT_ERROR_INVALID_ARGS;
	uint32_t key;
	const void *data;
	size_t size = 0;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);
	data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);

	if (data) {
		ot_rpc_mutex_lock();
		message = ot_res_tab_msg_get(key);

		if (message != NULL) {
			error = otMessageAppend(message, data, size);
		}

		ot_rpc_mutex_unlock();
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_APPEND);
		return;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_msg_udp_new(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	ot_rpc_res_tab_key key;
	otMessageSettings settings;
	otMessageSettings *p_settings = &settings;

	p_settings = ot_rpc_decode_message_settings(ctx, &settings);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_NEW_MESSAGE);
		return;
	}

	ot_rpc_mutex_lock();
	key = ot_res_tab_msg_alloc(otUdpNewMessage(openthread_get_default_instance(), p_settings));

	if (ot_res_tab_msg_get(key) == NULL) {
		key = 0;
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_uint(group, key);
}

static void ot_rpc_msg_length(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	ot_rpc_res_tab_key key;
	uint16_t length = 0;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_GET_LENGTH);
		return;
	}

	ot_rpc_mutex_lock();
	message = ot_res_tab_msg_get(key);

	if (message != NULL) {
		length = otMessageGetLength(message);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_uint(group, length);
}

static void ot_rpc_get_offset(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	ot_rpc_res_tab_key key;
	uint16_t offset = 0;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_GET_OFFSET);
		return;
	}

	ot_rpc_mutex_lock();
	message = ot_res_tab_msg_get(key);

	if (message != NULL) {
		offset = otMessageGetOffset(message);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_uint(group, offset);
}

static void ot_rpc_msg_read(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	ot_rpc_res_tab_key key;
	uint16_t offset;
	uint16_t length;
	otMessage *message;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint16_t message_length;
	uint16_t read = 0;

	key = nrf_rpc_decode_uint(ctx);
	offset = nrf_rpc_decode_uint(ctx);
	length = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_READ);
		return;
	}

	ot_rpc_mutex_lock();
	message = ot_res_tab_msg_get(key);

	if (message == NULL) {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
		nrf_rpc_encode_null(&rsp_ctx);
		goto exit;
	}

	/* Get the actual message size before reading to allocate as small buffer as necessary. */
	message_length = otMessageGetLength(message);
	offset = MIN(offset, message_length);
	length = MIN(length, message_length - offset);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, length + 3);

	if (!zcbor_bstr_start_encode(rsp_ctx.zs)) {
		goto exit;
	}

	read = otMessageRead(message, offset, rsp_ctx.zs->payload_mut, length);
	rsp_ctx.zs->payload_mut += read;

	if (!zcbor_bstr_end_encode(rsp_ctx.zs, NULL)) {
		goto exit;
	}

exit:
	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_get_thread_link_info(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key key;
	otMessage *message;
	otThreadLinkInfo link_info;
	otError error;
	size_t cbor_buffer_size;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	key = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_GET_THREAD_LINK_INFO);
		return;
	}

	ot_rpc_mutex_lock();
	message = ot_res_tab_msg_get(key);

	if (!message) {
		ot_rpc_mutex_unlock();
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_GET_THREAD_LINK_INFO);
		return;
	}

	error = otMessageGetThreadLinkInfo(message, &link_info);
	ot_rpc_mutex_unlock();

	cbor_buffer_size = 1;

	if (error == OT_ERROR_NONE) {
		cbor_buffer_size += 1 + sizeof(link_info.mPanId);
		cbor_buffer_size += 1 + sizeof(link_info.mChannel);
		cbor_buffer_size += 1 + sizeof(link_info.mRss);
		cbor_buffer_size += 1 + sizeof(link_info.mLqi);
		cbor_buffer_size += 2; /* mLinkSecurity + mIsDstPanIdBroadcast */
		cbor_buffer_size += 1 + sizeof(link_info.mTimeSyncSeq);
		cbor_buffer_size += 1 + sizeof(link_info.mNetworkTimeOffset);
		cbor_buffer_size += 1 + sizeof(link_info.mRadioType);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);

	nrf_rpc_encode_uint(&rsp_ctx, error);

	if (error == OT_ERROR_NONE) {
		nrf_rpc_encode_uint(&rsp_ctx, link_info.mPanId);
		nrf_rpc_encode_uint(&rsp_ctx, link_info.mChannel);
		nrf_rpc_encode_int(&rsp_ctx, link_info.mRss);
		nrf_rpc_encode_uint(&rsp_ctx, link_info.mLqi);
		nrf_rpc_encode_bool(&rsp_ctx, link_info.mLinkSecurity);
		nrf_rpc_encode_bool(&rsp_ctx, link_info.mIsDstPanIdBroadcast);
		nrf_rpc_encode_uint(&rsp_ctx, link_info.mTimeSyncSeq);
		nrf_rpc_encode_int64(&rsp_ctx, link_info.mNetworkTimeOffset);
		nrf_rpc_encode_uint(&rsp_ctx, link_info.mRadioType);
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_msg_tx_cb(const otMessage *aMessage, otError aError, void *aContext)
{
	ot_rpc_callback_id cb_id = (ot_rpc_callback_id)aContext;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc((otMessage *)aMessage);
	struct nrf_rpc_cbor_ctx ctx;

	if (msg_key == 0) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &ot_group, OT_RPC_CMD_MESSAGE_TX_CB,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 3 + sizeof(cb_id) + sizeof(msg_key) + sizeof(aError));
	nrf_rpc_encode_uint(&ctx, cb_id);
	nrf_rpc_encode_uint(&ctx, msg_key);
	nrf_rpc_encode_uint(&ctx, aError);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESSAGE_TX_CB, &ctx, nrf_rpc_rsp_decode_void,
				NULL);
	ot_rpc_mutex_lock();

	ot_res_tab_msg_free(msg_key);
}

static void ot_rpc_msg_register_tx_callback(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key key;
	uint32_t cb_id;
	otMessage *message;

	key = nrf_rpc_decode_uint(ctx);
	cb_id = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_REGISTER_TX_CALLBACK);
		return;
	}

	ot_rpc_mutex_lock();
	message = ot_res_tab_msg_get(key);

	if (!message) {
		ot_rpc_mutex_unlock();
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESSAGE_REGISTER_TX_CALLBACK);
		return;
	}

	otMessageRegisterTxCallback(message, (cb_id != 0) ? ot_rpc_msg_tx_cb : NULL, (void *)cb_id);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
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

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_get_thread_link_info,
			 OT_RPC_CMD_MESSAGE_GET_THREAD_LINK_INFO, ot_rpc_msg_get_thread_link_info,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_msg_register_tx_callback,
			 OT_RPC_CMD_MESSAGE_REGISTER_TX_CALLBACK, ot_rpc_msg_register_tx_callback,
			 NULL);
