/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_resource.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>

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
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(message);
	ot_socket_key soc_key = socket->mKey;

	if (msg_key == 0) {
		goto exit;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx,
			   sizeof(soc_key) + sizeof(msg_key) + sizeof(otMessageInfo) + 16);

	nrf_rpc_encode_uint(&ctx, soc_key);
	nrf_rpc_encode_uint(&ctx, msg_key);
	ot_rpc_encode_message_info(&ctx, message_info);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_UDP_RECEIVE_CB, &ctx, ot_rpc_decode_void,
				NULL);
	ot_rpc_mutex_lock();

exit:
	ot_res_tab_msg_free(msg_key); /* This is NULL safe. */
}

static void ot_rpc_udp_open(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	nrf_udp_socket *socket = NULL;
	otError error;
	ot_socket_key key;

	key = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_OPEN);
		return;
	}

	socket = nrf_udp_alloc_socket(key);

	if (socket == NULL) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	ot_rpc_mutex_lock();
	error = otUdpOpen(openthread_get_default_instance(), &socket->mSocket, handle_udp_receive,
			  socket);
	ot_rpc_mutex_unlock();

exit:
	if (error != OT_ERROR_NONE && socket != NULL) {
		nrf_udp_free_socket(key);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_send(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error;
	ot_socket_key soc_key;
	ot_rpc_res_tab_key msg_key;
	nrf_udp_socket *socket;
	otMessageInfo message_info;
	otMessage *message;

	memset(&message_info, 0, sizeof(message_info));

	soc_key = nrf_rpc_decode_uint(ctx);
	msg_key = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_SEND);
		return;
	}

	socket = nrf_udp_find_socket(soc_key);
	message = ot_res_tab_msg_get(msg_key);

	if (socket == NULL || message == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	ot_rpc_mutex_lock();
	error = otUdpSend(openthread_get_default_instance(), &socket->mSocket, message,
			  &message_info);
	ot_rpc_mutex_unlock();

	if (error == OT_ERROR_NONE) {
		ot_res_tab_msg_free(msg_key);
	}

exit:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_bind(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	ot_socket_key soc_key;
	nrf_udp_socket *socket;
	otSockAddr sock_name;
	otNetifIdentifier netif;

	soc_key = nrf_rpc_decode_uint(ctx);
	nrf_rpc_decode_buffer(ctx, sock_name.mAddress.mFields.m8,
			      sizeof(sock_name.mAddress.mFields.m8));
	sock_name.mPort = nrf_rpc_decode_uint(ctx);
	netif = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_BIND);
		return;
	}

	socket = nrf_udp_find_socket(soc_key);

	if (socket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	ot_rpc_mutex_lock();
	error = otUdpBind(openthread_get_default_instance(), &socket->mSocket, &sock_name, netif);
	ot_rpc_mutex_unlock();

exit:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_close(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data)
{
	otError error;
	ot_socket_key soc_key;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	nrf_udp_socket *socket;

	soc_key = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_CLOSE);
		return;
	}

	socket = nrf_udp_find_socket(soc_key);

	if (socket == NULL) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	ot_rpc_mutex_lock();
	error = otUdpClose(openthread_get_default_instance(), &socket->mSocket);
	ot_rpc_mutex_unlock();

	nrf_udp_free_socket(soc_key);

exit:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_connect(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otSockAddr sock_name;
	ot_socket_key soc_key;
	nrf_udp_socket *socket;

	soc_key = nrf_rpc_decode_uint(ctx);
	nrf_rpc_decode_buffer(ctx, sock_name.mAddress.mFields.m8,
			      sizeof(sock_name.mAddress.mFields.m8));
	sock_name.mPort = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_CONNECT);
		return;
	}

	socket = nrf_udp_find_socket(soc_key);

	if (socket == NULL) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	ot_rpc_mutex_lock();
	error = otUdpConnect(openthread_get_default_instance(), &socket->mSocket, &sock_name);
	ot_rpc_mutex_unlock();

exit:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_udp_is_open(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	bool open = false;
	ot_socket_key soc_key;
	nrf_udp_socket *socket;

	soc_key = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_UDP_IS_OPEN);
		return;
	}

	ot_rpc_mutex_lock();
	socket = nrf_udp_find_socket(soc_key);

	if (socket) {
		open = otUdpIsOpen(openthread_get_default_instance(), &socket->mSocket);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_bool(group, open);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_bind, OT_RPC_CMD_UDP_BIND, ot_rpc_udp_bind, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_close, OT_RPC_CMD_UDP_CLOSE, ot_rpc_udp_close, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_connect, OT_RPC_CMD_UDP_CONNECT, ot_rpc_udp_connect,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_is_open, OT_RPC_CMD_UDP_IS_OPEN, ot_rpc_udp_is_open,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_open, OT_RPC_CMD_UDP_OPEN, ot_rpc_udp_open, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_udp_send, OT_RPC_CMD_UDP_SEND, ot_rpc_udp_send, NULL);
