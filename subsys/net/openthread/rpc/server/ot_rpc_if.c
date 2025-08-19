/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>

#include <nrf_rpc_cbor.h>

#include <openthread/ip6.h>
#include <openthread/message.h>

#include <openthread.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ot_rpc, LOG_LEVEL_DBG);

static void ot_receive_handler(otMessage *message, void *context)
{
	const size_t len = otMessageGetLength(message);
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t buf_len;
	uint16_t read;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 10 + len);

	if (!zcbor_bstr_start_encode(ctx.zs)) {
		goto out;
	}

	buf_len = (ctx.zs[0].payload_end - ctx.zs[0].payload_mut);
	read = otMessageRead(message, 0, ctx.zs[0].payload_mut, buf_len);

	if (read != len) {
		nrf_rpc_encoder_invalid(&ctx);
		goto out;
	}

	ctx.zs->payload_mut += len;

	if (!zcbor_bstr_end_encode(ctx.zs, NULL)) {
		goto out;
	}

	LOG_DBG("Passing IPv6 packet to RPC client");

out:
	otMessageFree(message);
	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IF_RECEIVE, &ctx, ot_rpc_decode_void, NULL);
	ot_rpc_mutex_lock();
}

static void ot_rpc_cmd_if_enable(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	bool enable;
	otInstance *instance;

	enable = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_IF_ENABLE);
		return;
	}

	openthread_mutex_lock();
	instance = openthread_get_default_instance();
	otIp6SetReceiveCallback(instance, enable ? ot_receive_handler : NULL, NULL);
	openthread_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_if_enable, OT_RPC_CMD_IF_ENABLE, ot_rpc_cmd_if_enable,
			 NULL);

static void ot_rpc_cmd_if_send(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	const uint8_t *pkt_data;
	size_t pkt_data_len;
	otInstance *instance;
	otMessageSettings settings;
	otMessage *message;
	otError error;

	LOG_DBG("Sending IPv6 packet to OT");

	pkt_data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &pkt_data_len);

	if (!pkt_data) {
		goto out;
	}

	instance = openthread_get_default_instance();
	settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;
	settings.mLinkSecurityEnabled = true;

	ot_rpc_mutex_lock();
	message = otIp6NewMessage(instance, &settings);

	if (!message) {
		LOG_ERR("Failed to allocate message");
		goto unlock;
	}

	error = otMessageAppend(message, pkt_data, pkt_data_len);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to append message: %u", error);
		otMessageFree(message);
		goto unlock;
	}

	error = otIp6Send(instance, message);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to send message: %u", error);
		goto unlock;
	}

unlock:
	ot_rpc_mutex_unlock();

out:
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_IF_SEND);
		return;
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_if_send, OT_RPC_CMD_IF_SEND, ot_rpc_cmd_if_send,
			 NULL);
