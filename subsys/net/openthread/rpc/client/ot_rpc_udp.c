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
	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_encode_buffer(&ctx, aSockName->mAddress.mFields.m8,
			      sizeof(aSockName->mAddress.mFields.m8));
	nrf_rpc_encode_uint(&ctx, aSockName->mPort);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_CONNECT, &ctx, ot_rpc_decode_error,
				&error);

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
	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_CLOSE, &ctx, ot_rpc_decode_error, &error);

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

	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_encode_buffer(&ctx, aSockName->mAddress.mFields.m8,
			      sizeof(aSockName->mAddress.mFields.m8));
	nrf_rpc_encode_uint(&ctx, aSockName->mPort);
	nrf_rpc_encode_uint(&ctx, net_if);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_BIND, &ctx, ot_rpc_decode_error, &error);

	return error;
}

static void ot_rpc_cmd_udp_receive_cb(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otMessageInfo message_info;
	ot_rpc_res_tab_key msg_key = 0;
	ot_socket_key soc_key = 0;
	otUdpSocket *socket;

	soc_key = nrf_rpc_decode_uint(ctx);
	msg_key = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_RECEIVE_CB);
		return;
	}

	socket = (otUdpSocket *)soc_key;

	if (socket != NULL && socket->mHandler != NULL) {
		socket->mHandler(socket->mContext, (otMessage *)msg_key, &message_info);
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
	nrf_rpc_encode_uint(&ctx, key);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_OPEN, &ctx, ot_rpc_decode_error, &error);

	return error;
}

otError otUdpSend(otInstance *aInstance, otUdpSocket *aSocket, otMessage *aMessage,
		  const otMessageInfo *aMessageInfo)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_socket_key soc_key = (ot_socket_key)aSocket;
	ot_rpc_res_tab_key msg_key = (ot_rpc_res_tab_key)aMessage;
	otError error = OT_ERROR_NONE;

	ARG_UNUSED(aInstance);

	if (aSocket == NULL || aMessage == NULL || aMessageInfo == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(otMessageInfo) + 16);
	nrf_rpc_encode_uint(&ctx, soc_key);
	nrf_rpc_encode_uint(&ctx, msg_key);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_SEND, &ctx, ot_rpc_decode_error, &error);

	return error;
}
