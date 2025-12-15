/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_resource.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_coap.h>
#include <ot_rpc_common.h>
#include <ot_rpc_types.h>
#include <ot_rpc_lock.h>

#include <nrf_rpc_cbor.h>

#include <openthread/coap.h>

#include <zephyr/net/openthread.h>

struct ot_rpc_coap_resource_buf {
	struct ot_rpc_coap_resource_buf *next;
	otCoapResource resource;
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
};

static struct ot_rpc_coap_resource_buf *resources;

static struct ot_rpc_coap_resource_buf *find_coap_resource_by_uri(const char *uri)
{
	for (struct ot_rpc_coap_resource_buf *r = resources; r != NULL; r = r->next) {
		if (strcmp(r->resource.mUriPath, uri) == 0) {
			return r;
		}
	}

	return NULL;
}

static struct ot_rpc_coap_resource_buf *pop_coap_resource_by_uri(const char *uri)
{
	struct ot_rpc_coap_resource_buf **prev_next = &resources;

	for (struct ot_rpc_coap_resource_buf *r = resources; r != NULL;
	     prev_next = &r->next, r = r->next) {
		if (strcmp(r->resource.mUriPath, uri) == 0) {
			*prev_next = r->next;
			r->next = NULL;

			return r;
		}
	}

	return NULL;
}

static void ot_rpc_cmd_coap_new_message(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otMessageSettings settings_buf;
	otMessageSettings *settings;
	otMessage *message;
	ot_rpc_res_tab_key message_rep = 0;

	settings = ot_rpc_decode_message_settings(ctx, &settings_buf);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_NEW_MESSAGE);
		return;
	}

	ot_rpc_mutex_lock();
	message = otCoapNewMessage(openthread_get_default_instance(), settings);
	message_rep = ot_res_tab_msg_alloc(message);

	if ((message != NULL) && !message_rep) {
		/*
		 * Failed to allocate the message handle, so the ownership can't be passed to
		 * the RPC client. Therefore, free the message.
		 */
		otMessageFree(message);
	}

	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, message_rep);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_new_message, OT_RPC_CMD_COAP_NEW_MESSAGE,
			 ot_rpc_cmd_coap_new_message, NULL);

static void ot_rpc_cmd_coap_message_init(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otCoapType type;
	otCoapCode code;
	otMessage *message;

	message_rep = nrf_rpc_decode_uint(ctx);
	type = nrf_rpc_decode_uint(ctx);
	code = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT);
		return;
	}

	ot_rpc_mutex_lock();
	otCoapMessageInit(message, type, code);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_init, OT_RPC_CMD_COAP_MESSAGE_INIT,
			 ot_rpc_cmd_coap_message_init, NULL);

static void ot_rpc_cmd_coap_message_init_response(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key response_rep;
	ot_rpc_res_tab_key request_rep;
	otCoapType type;
	otCoapCode code;
	otMessage *response;
	const otMessage *request;
	otError error;

	response_rep = nrf_rpc_decode_uint(ctx);
	request_rep = nrf_rpc_decode_uint(ctx);
	type = nrf_rpc_decode_uint(ctx);
	code = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE);
		return;
	}

	response = ot_res_tab_msg_get(response_rep);
	request = ot_res_tab_msg_get(request_rep);

	if (!response || !request) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE);
		return;
	}

	ot_rpc_mutex_lock();
	error = otCoapMessageInitResponse(response, request, type, code);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_init_response,
			 OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE,
			 ot_rpc_cmd_coap_message_init_response, NULL);

static void ot_rpc_cmd_coap_message_append_uri_path_options(const struct nrf_rpc_group *group,
							    struct nrf_rpc_cbor_ctx *ctx,
							    void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);
	nrf_rpc_decode_str(ctx, uri, sizeof(uri));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS);
		return;
	}

	ot_rpc_mutex_lock();
	error = otCoapMessageAppendUriPathOptions(message, uri);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_append_uri_path_options,
			 OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS,
			 ot_rpc_cmd_coap_message_append_uri_path_options, NULL);

static void ot_rpc_cmd_coap_message_set_payload_marker(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER);
		return;
	}

	ot_rpc_mutex_lock();
	error = otCoapMessageSetPayloadMarker(message);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_set_payload_marker,
			 OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER,
			 ot_rpc_cmd_coap_message_set_payload_marker, NULL);

static void ot_rpc_cmd_coap_message_get_type(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessage *message;
	otCoapType type;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TYPE);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TYPE);
		return;
	}

	ot_rpc_mutex_lock();
	type = otCoapMessageGetType(message);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, type);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_type,
			 OT_RPC_CMD_COAP_MESSAGE_GET_TYPE, ot_rpc_cmd_coap_message_get_type, NULL);

static void ot_rpc_cmd_coap_message_get_code(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessage *message;
	otCoapCode code;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_CODE);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_CODE);
		return;
	}

	ot_rpc_mutex_lock();
	code = otCoapMessageGetCode(message);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, code);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_code,
			 OT_RPC_CMD_COAP_MESSAGE_GET_CODE, ot_rpc_cmd_coap_message_get_code, NULL);

static void ot_rpc_cmd_coap_message_get_message_id(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessage *message;
	uint16_t id;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID);
		return;
	}

	ot_rpc_mutex_lock();
	id = otCoapMessageGetMessageId(message);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, id);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_message_id,
			 OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID,
			 ot_rpc_cmd_coap_message_get_message_id, NULL);

static void ot_rpc_cmd_coap_message_get_token_length(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessage *message;
	uint8_t token_length;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH);
		return;
	}

	ot_rpc_mutex_lock();
	token_length = otCoapMessageGetTokenLength(message);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, token_length);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_token_length,
			 OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH,
			 ot_rpc_cmd_coap_message_get_token_length, NULL);

static void ot_rpc_cmd_coap_message_get_token(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessage *message;
	uint8_t token[OT_COAP_MAX_TOKEN_LENGTH];
	uint8_t token_length;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN);
		return;
	}

	ot_rpc_mutex_lock();
	token_length = otCoapMessageGetTokenLength(message);
	memcpy(token, otCoapMessageGetToken(message), token_length);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + token_length);
	nrf_rpc_encode_buffer(&rsp_ctx, token, token_length);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_token,
			 OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN, ot_rpc_cmd_coap_message_get_token,
			 NULL);

static void ot_rpc_cmd_coap_start(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	uint16_t port;
	otError error;

	port = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_START);
		return;
	}

	ot_rpc_mutex_lock();
	error = otCoapStart(openthread_get_default_instance(), port);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_start, OT_RPC_CMD_COAP_START,
			 ot_rpc_cmd_coap_start, NULL);

static void ot_rpc_cmd_coap_stop(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	otError error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_STOP);
		return;
	}

	ot_rpc_mutex_lock();
	error = otCoapStop(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_stop, OT_RPC_CMD_COAP_STOP, ot_rpc_cmd_coap_stop,
			 NULL);

static void ot_rpc_coap_resource_handler(void *aContext, otMessage *aMessage,
					 const otMessageInfo *aMessageInfo)
{
	ot_rpc_res_tab_key message_rep;
	const char *uri = aContext;
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	message_rep = ot_res_tab_msg_alloc(aMessage);

	if (!message_rep) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &ot_group,
			    OT_RPC_CMD_COAP_RESOURCE_HANDLER, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	cbor_buffer_size += 2 + strlen(uri);
	cbor_buffer_size += 1 + sizeof(ot_rpc_res_tab_key);	      /* aMessage */
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo); /* aMessageInfo */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_str(&ctx, uri, -1);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_RESOURCE_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
	ot_rpc_mutex_lock();

	ot_res_tab_msg_free(message_rep);
}

static void ot_rpc_cmd_coap_add_resource(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
	struct ot_rpc_coap_resource_buf *res_buf;

	nrf_rpc_decode_str(ctx, uri, sizeof(uri));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_ADD_RESOURCE);
		return;
	}

	ot_rpc_mutex_lock();

	if (find_coap_resource_by_uri(uri)) {
		goto out;
	}

	res_buf = malloc(sizeof(*res_buf));

	if (!res_buf) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_RECV, &ot_group, OT_RPC_CMD_COAP_ADD_RESOURCE,
			    NRF_RPC_PACKET_TYPE_CMD);
		goto out;
	}

	res_buf->resource.mUriPath = res_buf->uri;
	res_buf->resource.mHandler = ot_rpc_coap_resource_handler;
	res_buf->resource.mContext = res_buf->uri;
	res_buf->resource.mNext = NULL;
	strcpy(res_buf->uri, uri);
	res_buf->next = resources;
	resources = res_buf;

	otCoapAddResource(openthread_get_default_instance(), &res_buf->resource);

out:
	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_add_resource, OT_RPC_CMD_COAP_ADD_RESOURCE,
			 ot_rpc_cmd_coap_add_resource, NULL);

static void ot_rpc_cmd_coap_remove_resource(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
	struct ot_rpc_coap_resource_buf *res_buf;

	nrf_rpc_decode_str(ctx, uri, sizeof(uri));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_REMOVE_RESOURCE);
		return;
	}

	ot_rpc_mutex_lock();

	res_buf = pop_coap_resource_by_uri(uri);

	if (!res_buf) {
		goto out;
	}

	otCoapRemoveResource(openthread_get_default_instance(), &res_buf->resource);
	free(res_buf);

out:
	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_remove_resource, OT_RPC_CMD_COAP_REMOVE_RESOURCE,
			 ot_rpc_cmd_coap_remove_resource, NULL);

static void ot_rpc_coap_default_handler(void *aContext, otMessage *aMessage,
					const otMessageInfo *aMessageInfo)
{
	ot_rpc_res_tab_key message_rep;
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	message_rep = ot_res_tab_msg_alloc(aMessage);

	if (!message_rep) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &ot_group,
			    OT_RPC_CMD_COAP_DEFAULT_HANDLER, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	cbor_buffer_size += 1 + sizeof(ot_rpc_res_tab_key);	      /* aMessage */
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo); /* aMessageInfo */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_DEFAULT_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
	ot_rpc_mutex_lock();

	ot_res_tab_msg_free(message_rep);
}

static void ot_rpc_cmd_coap_set_default_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;

	enabled = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER);
		return;
	}

	ot_rpc_mutex_lock();
	otCoapSetDefaultHandler(openthread_get_default_instance(),
				enabled ? ot_rpc_coap_default_handler : NULL, NULL);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_set_default_handler,
			 OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, ot_rpc_cmd_coap_set_default_handler,
			 NULL);

static void ot_rpc_coap_response_handler(void *context, otMessage *message,
					 const otMessageInfo *message_info, otError error)
{
	ot_rpc_coap_request_key request_rep = (ot_rpc_coap_request_key)context;
	ot_rpc_res_tab_key message_rep = 0;
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	message_rep = ot_res_tab_msg_alloc(message);
	/*
	 * Ignore message handle allocation failure. It seems safer to call the client's response
	 * handler without the response (which indicates response timeout) than not to call the
	 * handler at all and make the client wait for the response indefinitely.
	 */

	cbor_buffer_size += 1 + sizeof(ot_rpc_coap_request_key);
	cbor_buffer_size += 1 + sizeof(ot_rpc_res_tab_key);
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(message_info);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, request_rep);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, message_info);
	nrf_rpc_encode_uint(&ctx, error);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_RESPONSE_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
	ot_rpc_mutex_lock();

	ot_res_tab_msg_free(message_rep);
}

static void ot_rpc_cmd_coap_send_request(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessageInfo message_info;
	ot_rpc_coap_request_key request_rep;
	otMessage *message;
	otCoapResponseHandler handler;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);
	request_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_SEND_REQUEST);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_SEND_REQUEST);
		return;
	}

	ot_rpc_mutex_lock();
	handler = (request_rep != 0) ? ot_rpc_coap_response_handler : NULL;
	error = otCoapSendRequest(openthread_get_default_instance(), message, &message_info,
				  handler, (void *)request_rep);

	if (error == OT_ERROR_NONE) {
		ot_res_tab_msg_free(message_rep);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_send_request, OT_RPC_CMD_COAP_SEND_REQUEST,
			 ot_rpc_cmd_coap_send_request, NULL);

static void ot_rpc_cmd_coap_send_response(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_res_tab_key message_rep;
	otMessageInfo message_info;
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_SEND_RESPONSE);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_COAP_SEND_RESPONSE);
		return;
	}

	ot_rpc_mutex_lock();
	error = otCoapSendResponse(openthread_get_default_instance(), message, &message_info);

	if (error == OT_ERROR_NONE) {
		ot_res_tab_msg_free(message_rep);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_send_response, OT_RPC_CMD_COAP_SEND_RESPONSE,
			 ot_rpc_cmd_coap_send_response, NULL);
