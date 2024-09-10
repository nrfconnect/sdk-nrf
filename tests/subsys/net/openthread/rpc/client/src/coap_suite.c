/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/coap.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

/* Shortest and longest possible CoAP URI */
#define SHORT_URI      'a'
#define CBOR_SHORT_URI 0x61, SHORT_URI
#define LONG_URI       STR_SEQUENCE(32)
#define CBOR_LONG_URI  0x78, 0x20, LONG_URI

/* Longest possible CoAP token */
#define TOKEN	   INT_SEQUENCE(OT_COAP_MAX_TOKEN_LENGTH)
#define CBOR_TOKEN 0x48, TOKEN

/* Fake functions */

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(ot_coap_handler, void *, otMessage *, const otMessageInfo *);
FAKE_VOID_FUNC(ot_coap_response_handler, void *, otMessage *, const otMessageInfo *, otError);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();
}

/* Test serialization of otCoapNewMessage() returning NULL */
ZTEST(ot_rpc_coap, test_otCoapNewMessage_0)
{
	otMessage *message;
	otMessageSettings settings = {
		.mLinkSecurityEnabled = false,
		.mPriority = OT_MESSAGE_PRIORITY_LOW,
	};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_NEW_MESSAGE, CBOR_FALSE, OT_MESSAGE_PRIORITY_LOW),
		RPC_RSP(0));
	message = otCoapNewMessage(NULL, &settings);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(message, NULL);
}

/* Test serialization of otCoapNewMessage() returning maximum allowed 0xffffffff */
ZTEST(ot_rpc_coap, test_otCoapNewMessage_max)
{
	otMessage *message;
	otMessageSettings settings = {
		.mLinkSecurityEnabled = true,
		.mPriority = OT_MESSAGE_PRIORITY_HIGH,
	};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_NEW_MESSAGE, CBOR_TRUE, OT_MESSAGE_PRIORITY_HIGH),
		RPC_RSP(CBOR_UINT32(UINT32_MAX)));
	message = otCoapNewMessage(NULL, &settings);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(message, (otMessage *)UINT32_MAX);
}

/* Test serialization of otCoapMessageInit() */
ZTEST(ot_rpc_coap, test_otCoapMessageInit)
{
	otMessage *message = (otMessage *)MSG_ADDR;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_INIT, CBOR_UINT32(MSG_ADDR),
					   OT_COAP_TYPE_RESET,
					   CBOR_UINT8(OT_COAP_CODE_PROXY_NOT_SUPPORTED)),
				   RPC_RSP());
	otCoapMessageInit(message, OT_COAP_TYPE_RESET, OT_COAP_CODE_PROXY_NOT_SUPPORTED);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otCoapMessageInitResponse() */
ZTEST(ot_rpc_coap, test_otCoapMessageInitResponse)
{
	otMessage *response = (otMessage *)MSG_ADDR;
	otMessage *request = (otMessage *)(MSG_ADDR - 1);
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_INIT_RESPONSE,
					   CBOR_UINT32(MSG_ADDR), CBOR_UINT32(MSG_ADDR - 1),
					   OT_COAP_TYPE_RESET,
					   CBOR_UINT8(OT_COAP_CODE_PROXY_NOT_SUPPORTED)),
				   RPC_RSP(OT_ERROR_INVALID_ARGS));
	error = otCoapMessageInitResponse(response, request, OT_COAP_TYPE_RESET,
					  OT_COAP_CODE_PROXY_NOT_SUPPORTED);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otCoapMessageAppendUriPathOptions(..., "a") */
ZTEST(ot_rpc_coap, test_otCoapMessageAppendUriPathOptions_short)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	char uri[] = {SHORT_URI, '\0'};
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS,
					   CBOR_UINT32(MSG_ADDR), CBOR_SHORT_URI),
				   RPC_RSP(OT_ERROR_INVALID_ARGS));
	error = otCoapMessageAppendUriPathOptions(message, uri);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otCoapMessageAppendUriPathOptions(..., "aaa...") */
ZTEST(ot_rpc_coap, test_otCoapMessageAppendUriPathOptions_long)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	char uri[] = {LONG_URI, '\0'};
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_APPEND_URI_PATH_OPTIONS,
					   CBOR_UINT32(MSG_ADDR), CBOR_LONG_URI),
				   RPC_RSP(OT_ERROR_INVALID_ARGS));
	error = otCoapMessageAppendUriPathOptions(message, uri);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otCoapMessageSetPayloadMarker() */
ZTEST(ot_rpc_coap, test_otCoapMessageSetPayloadMarker)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	otError error;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_SET_PAYLOAD_MARKER, CBOR_UINT32(MSG_ADDR)),
		RPC_RSP(OT_ERROR_INVALID_ARGS));
	error = otCoapMessageSetPayloadMarker(message);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otCoapMessageGetType() returning OT_COAP_TYPE_RESET */
ZTEST(ot_rpc_coap, test_otCoapMessageGetType)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	otCoapType type;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_TYPE, CBOR_UINT32(MSG_ADDR)),
				   RPC_RSP(OT_COAP_TYPE_RESET));
	type = otCoapMessageGetType(message);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(type, OT_COAP_TYPE_RESET);
}

/* Test serialization of otCoapMessageGetCode() returning OT_COAP_CODE_PROXY_NOT_SUPPORTED */
ZTEST(ot_rpc_coap, test_otCoapMessageGetCode)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	otCoapCode code;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_CODE, CBOR_UINT32(MSG_ADDR)),
				   RPC_RSP(CBOR_UINT8(OT_COAP_CODE_PROXY_NOT_SUPPORTED)));
	code = otCoapMessageGetCode(message);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(code, OT_COAP_CODE_PROXY_NOT_SUPPORTED);
}

/* Test serialization of otCoapMessageGetMessageId() returning max allowed 0xffff */
ZTEST(ot_rpc_coap, test_otCoapMessageGetMessageId)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	uint16_t id;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_MESSAGE_ID, CBOR_UINT32(MSG_ADDR)),
		RPC_RSP(CBOR_UINT16(UINT16_MAX)));
	id = otCoapMessageGetMessageId(message);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, UINT16_MAX);
}

/* Test serialization of otCoapMessageGetTokenLength() returning OT_COAP_MAX_TOKEN_LENGTH */
ZTEST(ot_rpc_coap, test_otCoapMessageGetTokenLength)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	uint8_t length;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN_LENGTH, CBOR_UINT32(MSG_ADDR)),
		RPC_RSP(CBOR_UINT8(OT_COAP_MAX_TOKEN_LENGTH)));
	length = otCoapMessageGetTokenLength(message);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(length, OT_COAP_MAX_TOKEN_LENGTH);
}

/* Test serialization of otCoapMessageGetToken() sequence of length OT_COAP_MAX_TOKEN_LENGTH */
ZTEST(ot_rpc_coap, test_otCoapMessageGetToken)
{
	otMessage *message = (otMessage *)MSG_ADDR;
	const uint8_t exp_token[] = {TOKEN};
	const uint8_t *token;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_MESSAGE_GET_TOKEN, CBOR_UINT32(MSG_ADDR)),
		RPC_RSP(CBOR_TOKEN));
	token = otCoapMessageGetToken(message);
	mock_nrf_rpc_tr_expect_done();

	zassert_mem_equal(token, exp_token, sizeof(exp_token));
}

/* Test serialization of otCoapStart(..., 0xffff) returning OT_ERROR_INVALID_STATE */
ZTEST(ot_rpc_coap, test_otCoapStart)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_START, CBOR_UINT16(UINT16_MAX)),
				   RPC_RSP(OT_ERROR_INVALID_STATE));
	error = otCoapStart(NULL, UINT16_MAX);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_STATE);
}

/* Test serialization of otCoapStop() returning OT_ERROR_INVALID_STATE */
ZTEST(ot_rpc_coap, test_otCoapStop)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_STOP), RPC_RSP(OT_ERROR_INVALID_STATE));
	error = otCoapStop(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_STATE);
}

/* Test serialization of otCoapAddResource() and otCoapRemoveResource() */
ZTEST(ot_rpc_coap, test_otCoapAddResource_otCoapRemoveResource)
{
	const char uri[] = {LONG_URI, '\0'};

	otCoapResource resource = {
		.mUriPath = uri,
		.mHandler = ot_coap_handler,
		.mContext = (void *)(UINT32_MAX - 1),
		.mNext = NULL,
	};

	/* Test serialization of otCoapAddResource() */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_ADD_RESOURCE, CBOR_LONG_URI), RPC_RSP());
	otCoapAddResource(NULL, &resource);
	mock_nrf_rpc_tr_expect_done();

	/* Test remote call of the resource handler */
	RESET_FAKE(ot_coap_handler);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_RESOURCE_HANDLER, CBOR_LONG_URI,
					CBOR_UINT32(MSG_ADDR), CBOR_MSG_INFO));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_coap_handler_fake.call_count, 1);
	zassert_equal(ot_coap_handler_fake.arg0_val, (void *)(UINT32_MAX - 1));
	zassert_equal(ot_coap_handler_fake.arg1_val, (otMessage *)MSG_ADDR);

	/* Test serialization of otCoapRemoveResource() */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_REMOVE_RESOURCE, CBOR_LONG_URI),
				   RPC_RSP());
	otCoapRemoveResource(NULL, &resource);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otCoapSetDefaultHandler() */
ZTEST(ot_rpc_coap, test_otCoapSetDefaultHandler)
{
	/* Test serialization of otCoapSetDefaultHandler() that takes non-null handler */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, CBOR_TRUE),
				   RPC_RSP());
	otCoapSetDefaultHandler(NULL, ot_coap_handler, (void *)(UINT32_MAX - 1));
	mock_nrf_rpc_tr_expect_done();

	/* Test remote call of the default handler */
	RESET_FAKE(ot_coap_handler);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_COAP_DEFAULT_HANDLER, CBOR_UINT32(MSG_ADDR), CBOR_MSG_INFO));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_coap_handler_fake.call_count, 1);
	zassert_equal(ot_coap_handler_fake.arg0_val, (void *)(UINT32_MAX - 1));
	zassert_equal(ot_coap_handler_fake.arg1_val, (otMessage *)MSG_ADDR);

	/* Test serialization of otCoapSetDefaultHandler() that takes null handler */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_SET_DEFAULT_HANDLER, CBOR_FALSE),
				   RPC_RSP());
	otCoapSetDefaultHandler(NULL, NULL, NULL);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otCoapSendRequest() returning OT_ERROR_NONE */
ZTEST(ot_rpc_coap, test_otCoapSendRequest)
{
	/*
	 * Request key is allocated by the otCoapSendRequest() encoder function but we know
	 * that the first free slot will be selected so it will be 1.
	 */
	ot_rpc_coap_request_key request_rep = 1;
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
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_SEND_REQUEST, CBOR_UINT32(MSG_ADDR),
					   CBOR_MSG_INFO, request_rep),
				   RPC_RSP(OT_ERROR_NONE));
	error = otCoapSendRequest(NULL, (otMessage *)MSG_ADDR, &message_info,
				  ot_coap_response_handler, (void *)(UINT32_MAX - 1));
	mock_nrf_rpc_tr_expect_done();

	/* Test remote call of the resource handler */
	RESET_FAKE(ot_coap_response_handler);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_COAP_RESPONSE_HANDLER, request_rep,
					CBOR_UINT32(MSG_ADDR), CBOR_MSG_INFO,
					CBOR_UINT8(OT_ERROR_RESPONSE_TIMEOUT)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(ot_coap_response_handler_fake.call_count, 1);
	zassert_equal(ot_coap_response_handler_fake.arg0_val, (void *)(UINT32_MAX - 1));
	zassert_equal(ot_coap_response_handler_fake.arg1_val, (otMessage *)MSG_ADDR);
	zassert_equal(ot_coap_response_handler_fake.arg3_val, OT_ERROR_RESPONSE_TIMEOUT);

	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otCoapSendRequest() returning OT_ERROR_INVALID_STATE */
ZTEST(ot_rpc_coap, test_otCoapSendRequest_failure)
{
	/*
	 * Request key is allocated by the otCoapSendRequest() encoder function but we know
	 * that the first free slot will be selected so it will be 1.
	 */
	ot_rpc_coap_request_key request_rep = 1;
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
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_COAP_SEND_REQUEST, CBOR_UINT32(MSG_ADDR),
					   CBOR_MSG_INFO, request_rep),
				   RPC_RSP(OT_ERROR_INVALID_STATE));
	error = otCoapSendRequest(NULL, (otMessage *)MSG_ADDR, &message_info,
				  ot_coap_response_handler, (void *)(UINT32_MAX - 1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_STATE);
}

/* Test serialization of otCoapSendResponse() returning OT_ERROR_INVALID_STATE */
ZTEST(ot_rpc_coap, test_otCoapSendResponse)
{
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
	otError error;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_COAP_SEND_RESPONSE, CBOR_UINT32(MSG_ADDR), CBOR_MSG_INFO),
		RPC_RSP(OT_ERROR_INVALID_STATE));
	error = otCoapSendResponse(NULL, (otMessage *)MSG_ADDR, &message_info);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_STATE);
}

ZTEST_SUITE(ot_rpc_coap, NULL, NULL, tc_setup, NULL, NULL);
