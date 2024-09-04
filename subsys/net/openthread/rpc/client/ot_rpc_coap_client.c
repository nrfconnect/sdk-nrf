/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_coap.h>
#include <ot_rpc_common.h>
#include <ot_rpc_types.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/coap.h>

struct ot_rpc_coap_request {
	otCoapResponseHandler handler;
	void *context;
};

static otCoapResource *resources;
static otCoapRequestHandler default_handler;
static void *default_handler_ctx;
static struct ot_rpc_coap_request requests[CONFIG_OPENTHREAD_RPC_CLIENT_NUM_SENT_COAP_REQUESTS];

static ot_rpc_coap_request_key ot_rpc_coap_request_alloc(otCoapResponseHandler handler,
							 void *context)
{
	for (ot_rpc_coap_request_key i = 0; i < ARRAY_SIZE(requests); i++) {
		if (requests[i].handler == NULL) {
			requests[i].handler = handler;
			requests[i].context = context;

			return i + 1;
		}
	}

	return 0;
}

otMessage *otCoapNewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_msg_key message_rep;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, OT_RPC_MESSAGE_SETTINGS_LENGTH);
	ot_rpc_encode_message_settings(&ctx, aSettings);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_COAP_NEW_MESSAGE, &ctx);
	nrf_rpc_rsp_decode_uint(&ot_group, &ctx, &message_rep, sizeof(message_rep));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return (otMessage *)message_rep;
}

void otCoapMessageInit(otMessage *aMessage, otCoapType aType, otCoapCode aCode)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;

	cbor_buffer_size += 1 + sizeof(ot_msg_key); /* aMessage */
	cbor_buffer_size += 1;			   /* aType */
	cbor_buffer_size += 1 + sizeof(aCode);	   /* aCode */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);
	nrf_rpc_encode_uint(&ctx, aType);
	nrf_rpc_encode_uint(&ctx, aCode);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_INIT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otCoapMessageInitResponse(otMessage *aResponse, const otMessage *aRequest, otCoapType aType,
				  otCoapCode aCode)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;
	otError error;

	cbor_buffer_size += 1 + sizeof(ot_msg_key); /* aResponse */
	cbor_buffer_size += 1 + sizeof(ot_msg_key); /* aRequest */
	cbor_buffer_size += 1;			   /* aType */
	cbor_buffer_size += 1 + sizeof(aCode);	   /* aCode */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aResponse);
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aRequest);
	nrf_rpc_encode_uint(&ctx, aType);
	nrf_rpc_encode_uint(&ctx, aCode);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otError otCoapMessageAppendUriPathOptions(otMessage *aMessage, const char *aUriPath)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;
	otError error;

	cbor_buffer_size += 1 + sizeof(ot_msg_key); /* aMessage */
	cbor_buffer_size += 2 + strlen(aUriPath);  /* aUriPath */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);
	nrf_rpc_encode_str(&ctx, aUriPath, -1);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otError otCoapMessageSetPayloadMarker(otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_msg_key));
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otCoapType otCoapMessageGetType(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	otCoapType type = 0;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_msg_key));
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_GET_TYPE, &ctx);
	nrf_rpc_rsp_decode_uint(&ot_group, &ctx, &type, sizeof(type));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return type;
}

otCoapCode otCoapMessageGetCode(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	otCoapCode code = 0;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_msg_key));
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_GET_CODE, &ctx);
	nrf_rpc_rsp_decode_uint(&ot_group, &ctx, &code, sizeof(code));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return code;
}

uint16_t otCoapMessageGetMessageId(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t id = 0;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_msg_key));
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID, &ctx,
				nrf_rpc_rsp_decode_u16, &id);

	return id;
}

uint8_t otCoapMessageGetTokenLength(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t token_length = 0;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_msg_key));
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH, &ctx,
				nrf_rpc_rsp_decode_u8, &token_length);

	return token_length;
}

const uint8_t *otCoapMessageGetToken(const otMessage *aMessage)
{
	struct nrf_rpc_cbor_ctx ctx;
	static uint8_t token[OT_COAP_MAX_TOKEN_LENGTH];

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_msg_key));
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN, &ctx);

	nrf_rpc_decode_buffer(&ctx, token, sizeof(token));

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN, NRF_RPC_PACKET_TYPE_RSP);
	}

	return token;
}

otError otCoapStart(otInstance *aInstance, uint16_t aPort)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(aPort));
	nrf_rpc_encode_uint(&ctx, aPort);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_START, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}

otError otCoapStop(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_STOP, &ctx, ot_rpc_decode_error, &error);

	return error;
}

/* Find the location of 'next' pointer that points to 'searched'. */
otCoapResource **find_coap_resource_prev_next(otCoapResource *searched)
{
	otCoapResource **prev_next = &resources;

	for (otCoapResource *r = resources; r != NULL; prev_next = &r->mNext, r = r->mNext) {
		if (r == searched) {
			return prev_next;
		}
	}

	return NULL;
}

/* Find the CoAP resource by URI. */
otCoapResource *find_coap_resource_by_uri(const char *uri)
{
	for (otCoapResource *r = resources; r != NULL; r = r->mNext) {
		if (strcmp(r->mUriPath, uri) == 0) {
			return r;
		}
	}

	return NULL;
}

void otCoapAddResource(otInstance *aInstance, otCoapResource *aResource)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool uri_subscribed;

	if (find_coap_resource_prev_next(aResource)) {
		/* Resource already added */
		return;
	}

	/*
	 * 1. Add the resource to the local list.
	 * 2. Subscribe to the URI at the RPC server when the first resource
	 * for the given URI is added.
	 */

	uri_subscribed = find_coap_resource_by_uri(aResource->mUriPath);
	aResource->mNext = resources;
	resources = aResource;

	if (uri_subscribed) {
		return;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2 + strlen(aResource->mUriPath));
	nrf_rpc_encode_str(&ctx, aResource->mUriPath, -1);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_ADD_RESOURCE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void otCoapRemoveResource(otInstance *aInstance, otCoapResource *aResource)
{
	struct nrf_rpc_cbor_ctx ctx;
	otCoapResource **prev_next = find_coap_resource_prev_next(aResource);

	if (!prev_next) {
		/* Resource already removed */
		return;
	}

	/*
	 * 1. Remove the resource from the local list.
	 * 2. Unsubscribe from the URI at the RPC server when the last resource
	 * for the given URI is removed.
	 */

	*prev_next = aResource->mNext;
	aResource->mNext = NULL;

	if (find_coap_resource_by_uri(aResource->mUriPath)) {
		return;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2 + strlen(aResource->mUriPath));
	nrf_rpc_encode_str(&ctx, aResource->mUriPath, -1);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_REMOVE_RESOURCE, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void ot_rpc_cmd_coap_resource_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char uri[OT_RPC_COAP_MAX_URI_LENGTH + 1];
	otMessage *message;
	otMessageInfo message_info;
	otCoapResource *resource;

	nrf_rpc_decode_str(ctx, uri, sizeof(uri));
	message = (otMessage *)nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_RESOURCE_HANDLER);
		return;
	}

	resource = find_coap_resource_by_uri(uri);

	if (resource && resource->mHandler != NULL) {
		resource->mHandler(resource->mContext, message, &message_info);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_resource_handler,
			 OT_RPC_CMD_COAP_RESOURCE_HANDLER, ot_rpc_cmd_coap_resource_handler, NULL);

void otCoapSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;

	/*
	 * 1. Store the handler along with its context locally.
	 * 2. Send a bool representing whether the handler is not null, signaling
	 *    if the RPC server should relay unhandled CoAP messages to the RPC client.
	 */
	default_handler = aHandler;
	default_handler_ctx = aContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, aHandler != NULL);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

static void ot_rpc_cmd_coap_default_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_msg_key message_rep;
	otMessageInfo message_info;

	message_rep = nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_DEFAULT_HANDLER);
		return;
	}

	if (default_handler != NULL) {
		default_handler(default_handler_ctx, (otMessage *)message_rep, &message_info);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_default_handler, OT_RPC_CMD_COAP_DEFAULT_HANDLER,
			 ot_rpc_cmd_coap_default_handler, NULL);

otError otCoapSendRequestWithParameters(otInstance *aInstance, otMessage *aMessage,
					const otMessageInfo *aMessageInfo,
					otCoapResponseHandler aHandler, void *aContext,
					const otCoapTxParameters *aTxParameters)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;
	ot_rpc_coap_request_key request_rep;
	otError error = OT_ERROR_PARSE;

	request_rep = ot_rpc_coap_request_alloc(aHandler, aContext);

	if (!request_rep) {
		return OT_ERROR_NO_BUFS;
	}

	cbor_buffer_size += 1 + sizeof(ot_msg_key); /* aMessage */
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo);
	cbor_buffer_size += 1 + sizeof(ot_rpc_coap_request_key);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);
	nrf_rpc_encode_uint(&ctx, request_rep);
	/* Ignore aTXParameters as it is NULL for otCoapSendRequest() that we only need for now */
	ARG_UNUSED(aTxParameters);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_SEND_REQUEST, &ctx, ot_rpc_decode_error,
				&error);

	if (error != OT_ERROR_NONE) {
		/* Release the request slot on failure. */
		requests[request_rep - 1].handler = NULL;
	}

	return error;
}

static void ot_rpc_cmd_coap_response_handler(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	ot_rpc_coap_request_key request_rep;
	otMessage *message;
	otMessageInfo message_info;
	otError error;
	struct ot_rpc_coap_request *request;

	request_rep = nrf_rpc_decode_uint(ctx);
	message = (otMessage *)nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);
	error = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_RESPONSE_HANDLER);
		return;
	}

	if (request_rep == 0 || request_rep > ARRAY_SIZE(requests)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_COAP_RESPONSE_HANDLER);
		return;
	}

	request = &requests[request_rep - 1];

	if (request->handler) {
		request->handler(request->context, message, &message_info, error);
		request->handler = NULL;
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_coap_response_handler,
			 OT_RPC_CMD_COAP_RESPONSE_HANDLER, ot_rpc_cmd_coap_response_handler, NULL);

otError otCoapSendResponseWithParameters(otInstance *aInstance, otMessage *aMessage,
					 const otMessageInfo *aMessageInfo,
					 const otCoapTxParameters *aTxParameters)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;
	otError error = OT_ERROR_PARSE;

	cbor_buffer_size += 1 + sizeof(ot_msg_key); /* aMessage */
	cbor_buffer_size += OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&ctx, (ot_msg_key)aMessage);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);
	/* Ignore aTXParameters as it is NULL for otCoapSendResponse() that we only need for now */
	ARG_UNUSED(aTxParameters);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_COAP_SEND_RESPONSE, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}
