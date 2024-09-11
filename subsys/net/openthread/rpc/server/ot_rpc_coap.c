/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_message.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_coap.h>
#include <ot_rpc_common.h>
#include <ot_rpc_types.h>

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
	ot_msg_key message_rep = 0;

	settings = ot_rpc_decode_message_settings(ctx, &settings_buf);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_NEW_MESSAGE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	message = otCoapNewMessage(openthread_get_default_instance(), settings);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!message) {
		goto out;
	}

	message_rep = ot_reg_msg_alloc(message);

	if (!message_rep) {
		/*
		 * If failed to allocate the message handle, the ownership can't be passed to the
		 * RPC client. Therefore, free the message.
		 */
		otMessageFree(message);
	}

out:
	nrf_rpc_rsp_send_uint(group, message_rep);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_new_message, OT_RPC_CMD_COAP_NEW_MESSAGE,
			 ot_rpc_cmd_coap_new_message, NULL);

static void ot_rpc_cmd_coap_message_init(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otCoapType type;
	otCoapCode code;
	otMessage *message;

	message_rep = nrf_rpc_decode_uint(ctx);
	type = nrf_rpc_decode_uint(ctx);
	code = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otCoapMessageInit(message, type, code);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_init, OT_RPC_CMD_COAP_MESSAGE_INIT,
			 ot_rpc_cmd_coap_message_init, NULL);

static void ot_rpc_cmd_coap_message_init_response(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key response_rep;
	ot_msg_key request_rep;
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
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE);
		return;
	}

	response = ot_msg_get(response_rep);
	request = ot_msg_get(request_rep);

	if (!response || !request) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapMessageInitResponse(response, request, type, code);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_init_response,
			 OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE,
			 ot_rpc_cmd_coap_message_init_response, NULL);

static void ot_rpc_cmd_coap_message_append_uri_path_options(const struct nrf_rpc_group *group,
							    struct nrf_rpc_cbor_ctx *ctx,
							    void *handler_data)
{
	ot_msg_key message_rep;
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);
	nrf_rpc_decode_str(ctx, uri, sizeof(uri));

	if (!nrf_rpc_decode_valid(ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapMessageAppendUriPathOptions(message, uri);
	nrf_rpc_cbor_decoding_done(group, ctx);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_append_uri_path_options,
			 OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS,
			 ot_rpc_cmd_coap_message_append_uri_path_options, NULL);

static void ot_rpc_cmd_coap_message_set_payload_marker(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	ot_msg_key message_rep;
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapMessageSetPayloadMarker(message);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_set_payload_marker,
			 OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER,
			 ot_rpc_cmd_coap_message_set_payload_marker, NULL);

static void ot_rpc_cmd_coap_message_get_type(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessage *message;
	otCoapType type;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TYPE);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TYPE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	type = otCoapMessageGetType(message);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, type);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_type,
			 OT_RPC_CMD_COAP_MESSAGE_GET_TYPE, ot_rpc_cmd_coap_message_get_type, NULL);

static void ot_rpc_cmd_coap_message_get_code(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessage *message;
	otCoapCode code;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_CODE);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_CODE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	code = otCoapMessageGetCode(message);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, code);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_code,
			 OT_RPC_CMD_COAP_MESSAGE_GET_CODE, ot_rpc_cmd_coap_message_get_code, NULL);

static void ot_rpc_cmd_coap_message_get_message_id(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessage *message;
	uint16_t id;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	id = otCoapMessageGetMessageId(message);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, id);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_message_id,
			 OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID,
			 ot_rpc_cmd_coap_message_get_message_id, NULL);

static void ot_rpc_cmd_coap_message_get_token_length(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	ot_msg_key message_rep;
	otMessage *message;
	uint8_t token_length;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	token_length = otCoapMessageGetTokenLength(message);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, token_length);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_message_get_token_length,
			 OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH,
			 ot_rpc_cmd_coap_message_get_token_length, NULL);

static void ot_rpc_cmd_coap_message_get_token(const struct nrf_rpc_group *group,
					      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessage *message;
	uint8_t token[OT_COAP_MAX_TOKEN_LENGTH];
	uint8_t token_length;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	message_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	token_length = otCoapMessageGetTokenLength(message);
	memcpy(token, otCoapMessageGetToken(message), token_length);
	openthread_api_mutex_unlock(openthread_get_default_context());

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
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_START);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapStart(openthread_get_default_instance(), port);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_start, OT_RPC_CMD_COAP_START,
			 ot_rpc_cmd_coap_start, NULL);

static void ot_rpc_cmd_coap_stop(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	otError error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_STOP);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapStop(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_stop, OT_RPC_CMD_COAP_STOP, ot_rpc_cmd_coap_stop,
			 NULL);

static void ot_rpc_coap_resource_handler(void *aContext, otMessage *aMessage,
					 const otMessageInfo *aMessageInfo)
{
	ot_msg_key message_rep;
	const char *uri = aContext;
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	message_rep = ot_reg_msg_alloc(aMessage);

	if (!message_rep) {
		return;
	}

	cbor_buffer_size += 2 + strlen(uri);
	cbor_buffer_size += 1 + sizeof(ot_msg_key);		      /* aMessage */
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo); /* aMessageInfo */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_str(&ctx, uri, -1);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_RESOURCE_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	ot_msg_free(message_rep);
}

static void ot_rpc_cmd_coap_add_resource(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
	struct ot_rpc_coap_resource_buf *res_buf;

	nrf_rpc_decode_str(ctx, uri, sizeof(uri));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_ADD_RESOURCE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());

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
	openthread_api_mutex_unlock(openthread_get_default_context());
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
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_REMOVE_RESOURCE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());

	res_buf = pop_coap_resource_by_uri(uri);

	if (!res_buf) {
		goto out;
	}

	otCoapRemoveResource(openthread_get_default_instance(), &res_buf->resource);
	free(res_buf);

out:
	openthread_api_mutex_unlock(openthread_get_default_context());
	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_remove_resource, OT_RPC_CMD_COAP_REMOVE_RESOURCE,
			 ot_rpc_cmd_coap_remove_resource, NULL);

static void ot_rpc_coap_default_handler(void *aContext, otMessage *aMessage,
					const otMessageInfo *aMessageInfo)
{
	ot_msg_key message_rep;
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	message_rep = ot_reg_msg_alloc(aMessage);

	if (!message_rep) {
		return;
	}

	cbor_buffer_size += 1 + sizeof(ot_msg_key);		      /* aMessage */
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo); /* aMessageInfo */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_DEFAULT_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	ot_msg_free(message_rep);
}

static void ot_rpc_cmd_coap_set_default_handler(const struct nrf_rpc_group *group,
						struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;

	enabled = nrf_rpc_decode_bool(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	otCoapSetDefaultHandler(openthread_get_default_instance(),
				enabled ? ot_rpc_coap_default_handler : NULL, NULL);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_set_default_handler,
			 OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, ot_rpc_cmd_coap_set_default_handler,
			 NULL);

static void ot_rpc_coap_response_handler(void *context, otMessage *message,
					 const otMessageInfo *message_info, otError error)
{
	ot_rpc_coap_request_key request_rep = (ot_rpc_coap_request_key)context;
	ot_msg_key message_rep = 0;
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	if (message) {
		message_rep = ot_reg_msg_alloc(message);

		if (!message_rep) {
			return;
		}
	}

	cbor_buffer_size += 1 + sizeof(ot_rpc_coap_request_key);
	cbor_buffer_size += 1 + sizeof(ot_msg_key);
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(message_info);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, request_rep);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, message_info);
	nrf_rpc_encode_uint(&ctx, error);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_RESPONSE_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	ot_msg_free(message_rep);
}

static void ot_rpc_cmd_coap_send_request(const struct nrf_rpc_group *group,
					 struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessageInfo message_info;
	ot_rpc_coap_request_key request_rep;
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);
	request_rep = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_SEND_REQUEST);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_SEND_REQUEST);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapSendRequest(openthread_get_default_instance(), message, &message_info,
				  ot_rpc_coap_response_handler, (void *)request_rep);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error == OT_ERROR_NONE) {
		ot_msg_free(message_rep);
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_send_request, OT_RPC_CMD_COAP_SEND_REQUEST,
			 ot_rpc_cmd_coap_send_request, NULL);

static void ot_rpc_cmd_coap_send_response(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessageInfo message_info;
	otMessage *message;
	otError error;

	message_rep = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_SEND_RESPONSE);
		return;
	}

	message = ot_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_SEND_RESPONSE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otCoapSendResponse(openthread_get_default_instance(), message, &message_info);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_send_response, OT_RPC_CMD_COAP_SEND_RESPONSE,
			 ot_rpc_cmd_coap_send_response, NULL);
