/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_message.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/udp.h>

#include <zephyr/net/openthread.h>

#define OT_UDP_SOCKETS 8

typedef struct nrf_udp_socket {
	ot_socket_key mKey;
	otUdpSocket mSocket;
} nrf_udp_socket;

static nrf_udp_socket sockets[OT_UDP_SOCKETS];

static nrf_udp_socket *nrf_udp_find_socket(ot_socket_key key)
{
	for (size_t i = 0; i < OT_UDP_SOCKETS; i++) {
		if (key == sockets[i].mKey) {
			return &sockets[i];
		}
	}

	return NULL;
}

static nrf_udp_socket *nrf_udp_alloc_socket(ot_socket_key key)
{
	nrf_udp_socket *ret = nrf_udp_find_socket(key);

	if (ret != NULL) {
		return ret;
	}

	ret = nrf_udp_find_socket(0);

	if (ret != NULL) {
		ret->mKey = key;
	}

	return ret;
}

static void nrf_udp_free_socket(ot_socket_key key)
{
	nrf_udp_socket *socket = nrf_udp_find_socket(key);

	if (socket != NULL) {
		memset(socket, 0, sizeof(nrf_udp_socket));
	}
}

static void handle_udp_receive(void *context, otMessage *message, const otMessageInfo *message_info)
{
	struct nrf_rpc_cbor_ctx ctx;
	nrf_udp_socket *socket = (nrf_udp_socket *)context;
	ot_msg_key msg_key = ot_reg_msg_alloc(message);
	ot_socket_key soc_key = socket->mKey;

	if (msg_key == 0) {
		goto exit;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx,
			   sizeof(soc_key) + sizeof(msg_key) + sizeof(otMessageInfo) + 16);

	if (!zcbor_uint_encode(ctx.zs, &soc_key, sizeof(soc_key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, &msg_key, sizeof(msg_key))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	if (!ot_rpc_encode_message_info(&ctx, message_info)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_UDP_RECEIVE_CB, &ctx);

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

exit:
	ot_msg_free(msg_key); /* This is NULL safe. */
}

static void ot_rpc_udp_open(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	bool decoded_ok;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	nrf_udp_socket *socket = NULL;
	otError error = OT_ERROR_NONE;
	ot_socket_key key;

	decoded_ok = zcbor_uint_decode(ctx->zs, &key, sizeof(key));

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	socket = nrf_udp_alloc_socket(key);

	if (socket == NULL) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otUdpOpen(openthread_get_default_instance(), &socket->mSocket, handle_udp_receive,
			  socket);
	openthread_api_mutex_unlock(openthread_get_default_context());

exit:
	if (error != OT_ERROR_NONE && socket != NULL) {
		nrf_udp_free_socket(key);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_send(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	bool decoded_ok;
	ot_socket_key soc_key = 0;
	ot_msg_key msg_key = 0;
	nrf_udp_socket *socket;
	otMessageInfo message_info;
	otMessage *message;

	memset(&message_info, 0, sizeof(message_info));

	decoded_ok = zcbor_uint_decode(ctx->zs, &soc_key, sizeof(soc_key));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	socket = nrf_udp_find_socket(soc_key);

	decoded_ok = zcbor_uint_decode(ctx->zs, &msg_key, sizeof(msg_key));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	message = ot_msg_get(msg_key);

	decoded_ok = ot_rpc_decode_message_info(ctx, &message_info);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (socket == NULL || message == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otUdpSend(openthread_get_default_instance(), &socket->mSocket, message,
			  &message_info);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error == OT_ERROR_NONE) {
		ot_msg_free(msg_key);
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_UDP_SEND);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_bind(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	otError error = OT_ERROR_NONE;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	bool decoded_ok;
	ot_socket_key soc_key = 0;
	nrf_udp_socket *socket;
	otSockAddr sock_name;
	otNetifIdentifier netif;
	struct zcbor_string zstr;

	decoded_ok = zcbor_uint_decode(ctx->zs, &soc_key, sizeof(soc_key));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	decoded_ok = zcbor_bstr_decode(ctx->zs, &zstr);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	memcpy(&sock_name.mAddress.mFields.m8, zstr.value, zstr.len);

	decoded_ok = zcbor_uint_decode(ctx->zs, &sock_name.mPort, sizeof(sock_name.mPort));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	decoded_ok = zcbor_uint_decode(ctx->zs, &netif, sizeof(netif));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	socket = nrf_udp_find_socket(soc_key);

	if (socket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otUdpBind(openthread_get_default_instance(), &socket->mSocket, &sock_name, netif);
	openthread_api_mutex_unlock(openthread_get_default_context());

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_UDP_BIND);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_close(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data)
{
	otError error = OT_ERROR_NONE;
	ot_socket_key soc_key = 0;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	bool decoded_ok;
	nrf_udp_socket *socket;

	decoded_ok = zcbor_uint_decode(ctx->zs, &soc_key, sizeof(soc_key));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	socket = nrf_udp_find_socket(soc_key);

	if (socket == NULL) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otUdpClose(openthread_get_default_instance(), &socket->mSocket);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_udp_free_socket(soc_key);

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_UDP_CLOSE);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_connect(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	otError error = OT_ERROR_NONE;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	bool decoded_ok;
	struct zcbor_string zstr;
	otSockAddr sock_name;
	ot_socket_key soc_key = 0;
	nrf_udp_socket *socket;

	decoded_ok = zcbor_uint_decode(ctx->zs, &soc_key, sizeof(soc_key));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	decoded_ok = zcbor_bstr_decode(ctx->zs, &zstr);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	memcpy(&sock_name.mAddress.mFields.m8, zstr.value, zstr.len);

	decoded_ok = zcbor_uint_decode(ctx->zs, &sock_name.mPort, sizeof(sock_name.mPort));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	socket = nrf_udp_find_socket(soc_key);

	if (socket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otUdpConnect(openthread_get_default_instance(), &socket->mSocket, &sock_name);
	openthread_api_mutex_unlock(openthread_get_default_context());

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_UDP_BIND);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_bind, OT_RPC_CMD_UDP_BIND, ot_rpc_udp_bind, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_close, OT_RPC_CMD_UDP_CLOSE, ot_rpc_udp_close, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_connect, OT_RPC_CMD_UDP_CONNECT, ot_rpc_udp_connect,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_open, OT_RPC_CMD_UDP_OPEN, ot_rpc_udp_open, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_send, OT_RPC_CMD_UDP_SEND, ot_rpc_udp_send, NULL);
