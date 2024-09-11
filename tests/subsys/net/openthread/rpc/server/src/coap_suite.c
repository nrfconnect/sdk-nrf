/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_message.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/coap.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

/* Longest possible CoAP URI */
#define URI	 STR_SEQUENCE(32)
#define CBOR_URI 0x78, 0x20, URI

/* Longest possible CoAP token */
#define TOKEN	   INT_SEQUENCE(OT_COAP_MAX_TOKEN_LENGTH)
#define CBOR_TOKEN 0x48, TOKEN

/* Fake functions */

FAKE_VALUE_FUNC(otMessage *, otUdpNewMessage, otInstance *, const otMessageSettings *);
FAKE_VALUE_FUNC(uint16_t, otMessageGetLength, const otMessage *);
FAKE_VALUE_FUNC(uint16_t, otMessageGetOffset, const otMessage *);
FAKE_VALUE_FUNC(uint16_t, otMessageRead, const otMessage *, uint16_t, void *, uint16_t);
FAKE_VOID_FUNC(otMessageFree, otMessage *);
FAKE_VALUE_FUNC(otError, otMessageAppend, otMessage *, const void *, uint16_t);
FAKE_VALUE_FUNC(otMessage *, otCoapNewMessage, otInstance *, const otMessageSettings *);
FAKE_VOID_FUNC(otCoapMessageInit, otMessage *, otCoapType, otCoapCode);
FAKE_VALUE_FUNC(otError, otCoapMessageInitResponse, otMessage *, const otMessage *, otCoapType,
		otCoapCode);
FAKE_VALUE_FUNC(otError, otCoapMessageAppendUriPathOptions, otMessage *, const char *);
FAKE_VALUE_FUNC(otError, otCoapMessageSetPayloadMarker, otMessage *);
FAKE_VALUE_FUNC(otCoapType, otCoapMessageGetType, const otMessage *);
FAKE_VALUE_FUNC(otCoapCode, otCoapMessageGetCode, const otMessage *);
FAKE_VALUE_FUNC(uint16_t, otCoapMessageGetMessageId, const otMessage *);
FAKE_VALUE_FUNC(uint8_t, otCoapMessageGetTokenLength, const otMessage *);
FAKE_VALUE_FUNC(const uint8_t *, otCoapMessageGetToken, const otMessage *);
FAKE_VALUE_FUNC(otError, otCoapStart, otInstance *, uint16_t);
FAKE_VALUE_FUNC(otError, otCoapStop, otInstance *);
FAKE_VOID_FUNC(otCoapAddResource, otInstance *, otCoapResource *);
FAKE_VOID_FUNC(otCoapRemoveResource, otInstance *, otCoapResource *);
FAKE_VOID_FUNC(otCoapSetDefaultHandler, otInstance *, otCoapRequestHandler, void *)
FAKE_VALUE_FUNC(otError, otCoapSendRequestWithParameters, otInstance *, otMessage *,
		const otMessageInfo *, otCoapResponseHandler, void *, const otCoapTxParameters *);
FAKE_VALUE_FUNC(otError, otCoapSendResponseWithParameters, otInstance *, otMessage *,
		const otMessageInfo *, const otCoapTxParameters *);

#define FOREACH_FAKE(f)                                                                            \
	f(otUdpNewMessage);                                                                        \
	f(otMessageGetLength);                                                                     \
	f(otMessageGetOffset);                                                                     \
	f(otMessageRead);                                                                          \
	f(otMessageFree);                                                                          \
	f(otMessageAppend);                                                                        \
	f(otCoapNewMessage);                                                                       \
	f(otCoapMessageInit);                                                                      \
	f(otCoapMessageInitResponse);                                                              \
	f(otCoapMessageAppendUriPathOptions);                                                      \
	f(otCoapMessageSetPayloadMarker);                                                          \
	f(otCoapMessageGetType);                                                                   \
	f(otCoapMessageGetCode);                                                                   \
	f(otCoapMessageGetMessageId);                                                              \
	f(otCoapMessageGetTokenLength);                                                            \
	f(otCoapMessageGetToken);                                                                  \
	f(otCoapStart);                                                                            \
	f(otCoapStop);                                                                             \
	f(otCoapAddResource);                                                                      \
	f(otCoapRemoveResource);                                                                   \
	f(otCoapSetDefaultHandler);                                                                \
	f(otCoapSendRequestWithParameters);                                                        \
	f(otCoapSendResponseWithParameters);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();

	/* Unit tests take up to two message slots. */
	ot_msg_free(1);
	ot_msg_free(2);
}

/*
 * Test reception of otCoapNewMessage() command.
 * Test serialization of the result: NULL.
 */
ZTEST(ot_rpc_coap, test_otCoapNewMessage_0)
{
	otCoapNewMessage_fake.return_val = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_COAP_NEW_MESSAGE, CBOR_FALSE, OT_MESSAGE_PRIORITY_LOW));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapNewMessage_fake.call_count, 1);
}

/*
 * Test reception of otCoapNewMessage() command.
 * Test serialization of the result: 0xffffffff.
 */
ZTEST(ot_rpc_coap, test_otCoapNewMessage_max)
{
	otCoapNewMessage_fake.return_val = (otMessage *)UINT32_MAX;

	/* otCoapNewMessage() handler sends slot ID (1) instead of the actual message pointer */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(1), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_COAP_NEW_MESSAGE, CBOR_TRUE, OT_MESSAGE_PRIORITY_HIGH));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapNewMessage_fake.call_count, 1);
}

/*
 * Test reception of otCoapMessageInit() command.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageInit)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_INIT, message_rep,
					OT_COAP_TYPE_RESET,
					CBOR_UINT8(OT_COAP_CODE_PROXY_NOT_SUPPORTED)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageInit_fake.call_count, 1);
	zassert_equal(otCoapMessageInit_fake.arg0_val, (otMessage *)MSG_ADDR);
	zassert_equal(otCoapMessageInit_fake.arg1_val, OT_COAP_TYPE_RESET);
	zassert_equal(otCoapMessageInit_fake.arg2_val, OT_COAP_CODE_PROXY_NOT_SUPPORTED);
}

/*
 * Test reception of otCoapMessageInitResponse() command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageInitResponse)
{
	ot_msg_key response_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);
	ot_msg_key request_rep = ot_reg_msg_alloc((otMessage *)(MSG_ADDR - 1));

	otCoapMessageInitResponse_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE, response_rep,
					request_rep, OT_COAP_TYPE_RESET,
					CBOR_UINT8(OT_COAP_CODE_PROXY_NOT_SUPPORTED)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageInitResponse_fake.call_count, 1);
	zassert_equal(otCoapMessageInitResponse_fake.arg0_val, (otMessage *)MSG_ADDR);
	zassert_equal(otCoapMessageInitResponse_fake.arg1_val, (otMessage *)(MSG_ADDR - 1));
	zassert_equal(otCoapMessageInitResponse_fake.arg2_val, OT_COAP_TYPE_RESET);
	zassert_equal(otCoapMessageInitResponse_fake.arg3_val, OT_COAP_CODE_PROXY_NOT_SUPPORTED);
}

/*
 * Test reception of otCoapMessageAppendUriPathOptions() command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageAppendUriPathOptions)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);
	char uri[] = {URI, '\0'};

	otCoapMessageAppendUriPathOptions_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS, message_rep, CBOR_URI));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageAppendUriPathOptions_fake.call_count, 1);
	zassert_equal(otCoapMessageAppendUriPathOptions_fake.arg0_val, (otMessage *)MSG_ADDR);
	zassert_str_equal(otCoapMessageAppendUriPathOptions_fake.arg1_val, uri);
}

/*
 * Test reception of otCoapMessageSetPayloadMarker() command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageSetPayloadMarker)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	otCoapMessageSetPayloadMarker_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER, message_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageSetPayloadMarker_fake.call_count, 1);
	zassert_equal(otCoapMessageSetPayloadMarker_fake.arg0_val, (otMessage *)MSG_ADDR);
}

/*
 * Test reception of otCoapMessageGetType() command.
 * Test serialization of the result: OT_COAP_TYPE_RESET.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageGetType)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	otCoapMessageGetType_fake.return_val = OT_COAP_TYPE_RESET;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_COAP_TYPE_RESET), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_TYPE, message_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageGetType_fake.call_count, 1);
	zassert_equal(otCoapMessageGetType_fake.arg0_val, (otMessage *)MSG_ADDR);
}

/*
 * Test reception of otCoapMessageGetCode() command.
 * Test serialization of the result: OT_COAP_TYPE_RESET.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageGetCode)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	otCoapMessageGetCode_fake.return_val = OT_COAP_CODE_PROXY_NOT_SUPPORTED;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(OT_COAP_CODE_PROXY_NOT_SUPPORTED)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_CODE, message_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageGetCode_fake.call_count, 1);
	zassert_equal(otCoapMessageGetCode_fake.arg0_val, (otMessage *)MSG_ADDR);
}

/*
 * Test reception of otCoapMessageGetMessageId() command.
 * Test serialization of the result: 0xffff.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageGetMessageId)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	otCoapMessageGetMessageId_fake.return_val = UINT16_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT16(UINT16_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID, message_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageGetMessageId_fake.call_count, 1);
	zassert_equal(otCoapMessageGetMessageId_fake.arg0_val, (otMessage *)MSG_ADDR);
}

/*
 * Test reception of otCoapMessageGetTokenLength() command.
 * Test serialization of the result: OT_COAP_MAX_TOKEN_LENGTH.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageGetTokenLength)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	otCoapMessageGetTokenLength_fake.return_val = OT_COAP_MAX_TOKEN_LENGTH;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_COAP_MAX_TOKEN_LENGTH), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH, message_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageGetTokenLength_fake.call_count, 1);
	zassert_equal(otCoapMessageGetTokenLength_fake.arg0_val, (otMessage *)MSG_ADDR);
}

/*
 * Test reception of otCoapMessageGetToken() command.
 * Test serialization of the result of length OT_COAP_MAX_TOKEN_LENGTH.
 */
ZTEST(ot_rpc_coap, test_otCoapMessageGetToken)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);
	uint8_t token[] = {TOKEN};

	otCoapMessageGetToken_fake.return_val = token;
	otCoapMessageGetTokenLength_fake.return_val = OT_COAP_MAX_TOKEN_LENGTH;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_TOKEN), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN, message_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapMessageGetToken_fake.call_count, 1);
	zassert_equal(otCoapMessageGetToken_fake.arg0_val, (otMessage *)MSG_ADDR);
}

/*
 * Test reception of otCoapStart() command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_coap, test_otCoapStart)
{
	otCoapStart_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_START, CBOR_UINT16(UINT16_MAX)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapStart_fake.call_count, 1);
	zassert_equal(otCoapStart_fake.arg1_val, UINT16_MAX);
}

/*
 * Test reception of otCoapStop() command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_coap, test_otCoapStop)
{
	otCoapStop_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_STOP));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapStop_fake.call_count, 1);
}

static void otCoapAddResource_custom_func(otInstance *instance, otCoapResource *resource)
{
	char uri[] = {URI, '\0'};

	zassert_not_null(resource);
	zexpect_str_equal(resource->mUriPath, uri);
}

static void otCoapRemoveResource_custom_func(otInstance *instance, otCoapResource *resource)
{
	char uri[] = {URI, '\0'};

	zassert_not_null(resource);
	zexpect_str_equal(resource->mUriPath, uri);
}

/*
 * Test reception of otCoapAddResource() and otCoapRemoveResource() commands.
 */
ZTEST(ot_rpc_coap, test_otCoapAddResource_otCoapRemoveResource)
{
	/*
	 * Message key is allocated by the default handler encoder function but we know that
	 * the first free slot will be selected so it will be 1.
	 */
	ot_msg_key message_rep = 1;
	otMessageInfo message_info = {
		.mSockAddr = {.mFields.m8 = {ADDR_1}},
		.mPeerAddr = {.mFields.m8 = {ADDR_2}},
		.mSockPort = PORT_1,
		.mPeerPort = PORT_2,
		.mHopLimit = HOP_LIMIT,
		.mEcn = 3,
		.mIsHostInterface = true,
		.mAllowZeroHopLimit = true,
		.mMulticastLoop = true,
	};

	/* Test reception of otCoapAddResource() */
	otCoapAddResource_fake.custom_fake = otCoapAddResource_custom_func;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_ADD_RESOURCE, CBOR_URI));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapAddResource_fake.call_count, 1);

	/* Test serialization of the resource handler call */
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_RESOURCE_HANDLER, CBOR_URI, message_rep, CBOR_MSG_INFO),
		RPC_RSP());
	otCoapAddResource_fake.arg1_val->mHandler(otCoapAddResource_fake.arg1_val->mContext,
						  (otMessage *)MSG_ADDR, &message_info);
	mock_nrf_rpc_tr_expect_done();

	/* Test reception of otCoapRemoveResource() */
	otCoapRemoveResource_fake.custom_fake = otCoapRemoveResource_custom_func;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_REMOVE_RESOURCE, CBOR_URI));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapRemoveResource_fake.call_count, 1);
}

/*
 * Test reception of otCoapSetDefaultHandler() command.
 */
ZTEST(ot_rpc_coap, test_otCoapSetDefaultHandler)
{
	/*
	 * Message key is allocated by the default handler encoder function but we know that
	 * the first free slot will be selected so it will be 1.
	 */
	ot_msg_key message_rep = 1;
	otMessageInfo message_info = {
		.mSockAddr = {.mFields.m8 = {ADDR_1}},
		.mPeerAddr = {.mFields.m8 = {ADDR_2}},
		.mSockPort = PORT_1,
		.mPeerPort = PORT_2,
		.mHopLimit = HOP_LIMIT,
		.mEcn = 3,
		.mIsHostInterface = true,
		.mAllowZeroHopLimit = true,
		.mMulticastLoop = true,
	};

	/* Test reception of otCoapSetDefaultHandler() command that sets the default handler */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, CBOR_TRUE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapSetDefaultHandler_fake.call_count, 1);
	zassert_not_null(otCoapSetDefaultHandler_fake.arg1_val);

	/* Test serialization of the default handler call */
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_DEFAULT_HANDLER, message_rep, CBOR_MSG_INFO), RPC_RSP());
	otCoapSetDefaultHandler_fake.arg1_val(otCoapSetDefaultHandler_fake.arg2_val,
					      (otMessage *)MSG_ADDR, &message_info);
	mock_nrf_rpc_tr_expect_done();

	/* Test reception of otCoapSetDefaultHandler() command that unsets the default handler */
	RESET_FAKE(otCoapSetDefaultHandler);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, CBOR_FALSE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapSetDefaultHandler_fake.call_count, 1);
	zassert_is_null(otCoapSetDefaultHandler_fake.arg1_val);
}

/*
 * Test reception of otCoapSendRequest().
 */
ZTEST(ot_rpc_coap, test_otCoapSendRequest)
{
	ot_rpc_coap_request_key request_rep = 3;
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);
	/*
	 * The request message slot is freed when otCoapSendRequest() succeeds,
	 * so we know that the same slot will be used to relay the response message.
	 */
	ot_msg_key response_rep = message_rep;
	otMessageInfo message_info = {
		.mSockAddr = {.mFields.m8 = {ADDR_1}},
		.mPeerAddr = {.mFields.m8 = {ADDR_2}},
		.mSockPort = PORT_1,
		.mPeerPort = PORT_2,
		.mHopLimit = HOP_LIMIT,
		.mEcn = 3,
		.mIsHostInterface = true,
		.mAllowZeroHopLimit = true,
		.mMulticastLoop = true,
	};

	otCoapSendRequestWithParameters_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_COAP_SEND_REQUEST, message_rep, CBOR_MSG_INFO, request_rep));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapSendRequestWithParameters_fake.call_count, 1);
	zassert_equal(otCoapSendRequestWithParameters_fake.arg1_val, (otMessage *)MSG_ADDR);
	zassert_not_null(otCoapSendRequestWithParameters_fake.arg2_val);
	zassert_not_null(otCoapSendRequestWithParameters_fake.arg3_val);

	/* Test serialization of the response handler call */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_RESPONSE_HANDLER, request_rep,
					   response_rep, CBOR_MSG_INFO, OT_ERROR_PARSE),
				   RPC_RSP());
	otCoapSendRequestWithParameters_fake.arg3_val(otCoapSendRequestWithParameters_fake.arg4_val,
						      (otMessage *)MSG_ADDR, &message_info,
						      OT_ERROR_PARSE);
	mock_nrf_rpc_tr_expect_done();
}

/*
 * Test reception of otCoapSendResponse().
 */
ZTEST(ot_rpc_coap, test_otCoapSendResponse)
{
	ot_msg_key message_rep = ot_reg_msg_alloc((otMessage *)MSG_ADDR);

	otCoapSendResponseWithParameters_fake.return_val = OT_ERROR_INVALID_STATE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_STATE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_SEND_RESPONSE, message_rep, CBOR_MSG_INFO));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otCoapSendResponseWithParameters_fake.call_count, 1);
	zassert_equal(otCoapSendResponseWithParameters_fake.arg1_val, (otMessage *)MSG_ADDR);
	zassert_not_null(otCoapSendResponseWithParameters_fake.arg2_val);
}

ZTEST_SUITE(ot_rpc_coap, NULL, NULL, tc_setup, NULL, NULL);
