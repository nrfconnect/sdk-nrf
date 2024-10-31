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

#include <openthread/error.h>
#include <openthread/udp.h>

#include <string.h>

otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	otError error = OT_ERROR_NONE;

	if (aLength == 0 || aBuf == NULL) {
		return error;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, aLength + sizeof(key) + 5);
	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_encode_buffer(&ctx, aBuf, aLength);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESSAGE_APPEND, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}

otMessage *otUdpNewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
	otMessage *msg = NULL;
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(otMessageSettings) + 3);

	nrf_rpc_encode_buffer(&ctx, (const void *)aSettings, sizeof(otMessageSettings));

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_UDP_NEW_MESSAGE, &ctx);

	key = nrf_rpc_decode_uint(&ctx);
	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_UDP_NEW_MESSAGE);
		return msg;
	}

	msg = (otMessage *)key;
	return msg;
}

void otMessageFree(otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(ot_msg_key) + 1);

	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_FREE, &ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_MESSAGE_FREE);
	}
}

uint16_t otMessageGetLength(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	uint16_t ret = 0;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(uint32_t) + 1);

	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_GET_LENGTH, &ctx);

	ret = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_MESSAGE_GET_LENGTH);
		return 0;
	}

	return ret;
}

uint16_t otMessageGetOffset(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	uint16_t ret = 0;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(uint32_t) + 1);

	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_GET_OFFSET, &ctx);

	ret = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_MESSAGE_GET_OFFSET);
		return 0;
	}

	return ret;
}

uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	size_t size = 0;
	const void *buf = NULL;

	if (aLength == 0 || aBuf == NULL || aMessage == NULL) {
		return 0;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(key) + sizeof(aOffset) + sizeof(aLength) + 5);

	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_encode_uint(&ctx, aOffset);
	nrf_rpc_encode_uint(&ctx, aLength);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_READ, &ctx);

	buf = nrf_rpc_decode_buffer_ptr_and_size(&ctx, &size);

	if (buf && size) {
		memcpy(aBuf, buf, MIN(size, aLength));
	}

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_MESSAGE_READ);
		return 0;
	}

	return MIN(size, aLength);
}
