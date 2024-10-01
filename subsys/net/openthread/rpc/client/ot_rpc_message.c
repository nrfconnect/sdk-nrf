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
		error = OT_ERROR_NONE;
		goto exit;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, aLength + sizeof(key) + 5);

	if (!zcbor_uint_encode(ctx.zs, &key, sizeof(key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_bstr_encode_ptr(ctx.zs, aBuf, aLength)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESSAGE_APPEND, &ctx, ot_rpc_decode_error,
				&error);

exit:
	return error;
}

otMessage *otUdpNewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
	otMessage *msg = NULL;
	bool decoded_ok;
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = 0;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(otMessageSettings) + 3);

	nrf_rpc_encode_buffer(&ctx, (const void *)aSettings, sizeof(otMessageSettings));

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_UDP_NEW_MESSAGE, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, &key, sizeof(key));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, OT_RPC_CMD_UDP_NEW_MESSAGE,
			    NRF_RPC_PACKET_TYPE_RSP);
	}

	msg = (otMessage *)key;
	return msg;
}

void otMessageFree(otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(ot_msg_key) + 1);

	if (!zcbor_uint32_encode(ctx.zs, &key)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_FREE, &ctx);

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
}

uint16_t otMessageGetLength(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	uint16_t ret = 0;
	bool decoded_ok;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(uint32_t) + 1);

	if (!zcbor_uint32_encode(ctx.zs, &key)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_GET_LENGTH, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, &ret, sizeof(ret));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_MESSAGE_GET_LENGTH, NRF_RPC_PACKET_TYPE_RSP);
	}

exit:
	return ret;
}

uint16_t otMessageGetOffset(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	uint16_t ret = 0;
	bool decoded_ok;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(uint32_t) + 2);

	if (!zcbor_uint32_encode(ctx.zs, &key)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_GET_OFFSET, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, &ret, sizeof(ret));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_MESSAGE_GET_OFFSET, NRF_RPC_PACKET_TYPE_RSP);
	}

exit:
	return ret;
}

uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key key = (ot_msg_key)aMessage;
	uint16_t ret = 0;
	struct zcbor_string zst;
	bool decoded_ok;

	if (aLength == 0 || aBuf == NULL || aMessage == NULL) {
		return 0;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(key) + sizeof(aOffset) + sizeof(aLength) + 5);

	if (!zcbor_uint_encode(ctx.zs, &key, sizeof(key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, &aOffset, sizeof(aOffset))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, &aLength, sizeof(aLength))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESSAGE_READ, &ctx);

	if (zcbor_nil_expect(ctx.zs, NULL)) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		goto exit;
	}

	if (ctx.zs->constant_state->error != ZCBOR_ERR_WRONG_TYPE) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		goto exit;
	}

	zcbor_pop_error(ctx.zs);

	decoded_ok = zcbor_bstr_decode(ctx.zs, &zst);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, OT_RPC_CMD_MESSAGE_READ,
			    NRF_RPC_PACKET_TYPE_RSP);
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		goto exit;
	}

	ret = zst.len < aLength ? zst.len : aLength;

	memcpy(aBuf, zst.value, ret);

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

exit:
	return ret;
}
