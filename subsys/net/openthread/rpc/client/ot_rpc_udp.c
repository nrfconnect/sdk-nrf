/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_encode.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/message.h>
#include <openthread/error.h>

#include <openthread/udp.h>

#include <string.h>

otError otUdpConnect(otInstance *aInstance, otUdpSocket *aSocket, const otSockAddr *aSockName)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_socket_key key = (ot_socket_key)aSocket;
	otError error = OT_ERROR_NONE;

	ARG_UNUSED(aInstance);

	if (aSocket == NULL || aSockName == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(key) + sizeof(otSockAddr) + 6);

	if (!zcbor_uint_encode(ctx.zs, &key, sizeof(key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_bstr_encode_ptr(ctx.zs, (const char *)&aSockName->mAddress.mFields.m8,
				   sizeof(aSockName->mAddress.mFields.m8))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, (const char *)&aSockName->mPort, sizeof(aSockName->mPort))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_CONNECT, &ctx, ot_rpc_decode_error,
				&error);

exit:
	return error;
}

otError otUdpClose(otInstance *aInstance, otUdpSocket *aSocket)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_socket_key key = (ot_socket_key)aSocket;
	otError error = OT_ERROR_NONE;

	ARG_UNUSED(aInstance);

	if (aSocket == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(key) + 2);

	if (!zcbor_uint_encode(ctx.zs, &key, sizeof(key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_CLOSE, &ctx, ot_rpc_decode_error, &error);

exit:
	return error;
}

otError otUdpBind(otInstance *aInstance, otUdpSocket *aSocket, const otSockAddr *aSockName,
		  otNetifIdentifier aNetif)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_socket_key key = (ot_socket_key)aSocket;
	otError error = OT_ERROR_NONE;
	uint32_t net_if = aNetif;

	ARG_UNUSED(aInstance);

	if (aSocket == NULL || aSockName == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(key) + sizeof(otSockAddr) + sizeof(net_if) + 6);

	if (!zcbor_uint_encode(ctx.zs, &key, sizeof(key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_bstr_encode_ptr(ctx.zs, (const char *)&aSockName->mAddress.mFields.m8,
				   sizeof(aSockName->mAddress.mFields.m8))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, (const char *)&aSockName->mPort, sizeof(aSockName->mPort))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, &net_if, sizeof(net_if))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_BIND, &ctx, ot_rpc_decode_error, &error);

exit:
	return error;
}

static void ot_rpc_cmd_udp_receive_cb(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool decoding_ok;
	otMessageInfo message_info;
	ot_msg_key msg_key = 0;
	ot_socket_key soc_key = 0;
	otUdpSocket *socket;

	decoding_ok = zcbor_uint_decode(ctx->zs, &soc_key, sizeof(soc_key));

	if (!decoding_ok) {
		goto exit;
	}

	decoding_ok = zcbor_uint_decode(ctx->zs, &msg_key, sizeof(msg_key));

	if (!decoding_ok) {
		goto exit;
	}

	decoding_ok = ot_rpc_decode_message_info(ctx, &message_info);

	if (!decoding_ok) {
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	socket = (otUdpSocket *)soc_key;

	if (socket == NULL || socket->mHandler == NULL) {
		goto exit;
	}

	socket->mHandler(socket->mContext, (otMessage *)msg_key, &message_info);

exit:

	if (!decoding_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_udp_receive_cb, OT_RPC_CMD_UDP_RECEIVE_CB,
			 ot_rpc_cmd_udp_receive_cb, NULL);

otError otUdpOpen(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback,
		  void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_socket_key key = (ot_socket_key)aSocket;
	otError error = OT_ERROR_NONE;

	ARG_UNUSED(aInstance);

	if (aSocket == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	aSocket->mContext = aContext;
	aSocket->mHandler = aCallback;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(key) + 2);

	if (!zcbor_uint_encode(ctx.zs, &key, sizeof(key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_OPEN, &ctx, ot_rpc_decode_error, &error);

exit:
	return error;
}

otError otUdpSend(otInstance *aInstance, otUdpSocket *aSocket, otMessage *aMessage,
		  const otMessageInfo *aMessageInfo)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_socket_key soc_key = (ot_socket_key)aSocket;
	ot_msg_key msg_key = (ot_msg_key)aMessage;
	otError error = OT_ERROR_NONE;

	ARG_UNUSED(aInstance);

	if (aSocket == NULL || aMessage == NULL || aMessageInfo == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(otMessageInfo) + 16);

	if (!zcbor_uint_encode(ctx.zs, &soc_key, sizeof(soc_key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, &msg_key, sizeof(msg_key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!ot_rpc_encode_message_info(&ctx, aMessageInfo)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_SEND, &ctx, ot_rpc_decode_error, &error);
exit:
	return error;
}
