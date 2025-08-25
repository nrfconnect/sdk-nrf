/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common_fakes.h"

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_lock.h>
#include <ot_rpc_resource.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/message.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

/* Message TX callback ID */
#define MSG_TX_CB_ID (UINT32_MAX - 1)

#define LONG_BSTR_LEN 256

FAKE_VOID_FUNC(otMessageRegisterTxCallback, otMessage *, otMessageTxCallback, void *);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

#define FOREACH_FAKE(f)                                                                            \
	f(otMessageGetLength);                                                                     \
	f(otMessageGetOffset);                                                                     \
	f(otMessageGetThreadLinkInfo);                                                             \
	f(otMessageRead);                                                                          \
	f(otMessageFree);                                                                          \
	f(otMessageAppend);                                                                        \
	f(otUdpNewMessage);                                                                        \
	f(otMessageRegisterTxCallback);

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

static void tc_cleanup(void *f)
{
	/* This suite uses one message at most. */
	ot_res_tab_msg_free(1);
}

static otMessage *udp_new_message_failed_fake(otInstance *instance,
					      const otMessageSettings *settings)
{
	zassert_true(settings->mLinkSecurityEnabled);
	zassert_equal(settings->mPriority, 40);

	return NULL;
}

ZTEST(ot_rpc_message, test_otUdpNewMessage_failing)
{
	const uint8_t prio = 40;

	otUdpNewMessage_fake.return_val = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_NULL));

	zassert_equal(otUdpNewMessage_fake.call_count, 1);
	zassert_is_null(otUdpNewMessage_fake.arg1_val);

	otUdpNewMessage_fake.custom_fake = udp_new_message_failed_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_TRUE, CBOR_UINT8(prio)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otUdpNewMessage_fake.call_count, 2);
}

static otMessage *udp_new_message_free_fake(otInstance *instance, const otMessageSettings *settings)
{
	zassert_true(settings->mLinkSecurityEnabled);
	zassert_equal(settings->mPriority, 40);

	return (otMessage *)1;
}

ZTEST(ot_rpc_message, test_otUdpNewMessage_free_working)
{
	const uint8_t prio = 40;

	otUdpNewMessage_fake.return_val = (otMessage *)1;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(1), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_NULL));

	zassert_equal(otUdpNewMessage_fake.call_count, 1);
	zassert_is_null(otUdpNewMessage_fake.arg1_val);

	otUdpNewMessage_fake.custom_fake = udp_new_message_free_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(2), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_TRUE, CBOR_UINT8(prio)));

	zassert_equal(otUdpNewMessage_fake.call_count, 2);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_FREE, 1));

	zassert_equal(otMessageFree_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_FREE, 2));

	zassert_equal(otMessageFree_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMessageFree_fake.call_count, 2);
}

/*
 * Test ot_res_tab_msg functions.
 */
ZTEST(ot_rpc_message, test_ot_res_tab_msg)
{
	/* Verify that the configured number of messages can be allocated. */
	for (size_t i = 0; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL; i++) {
		zassert_equal(ot_res_tab_msg_alloc((otMessage *)(MSG_ADDR - i)), i + 1);
	}

	/* Verify that no more messages can be allocated. */
	zassert_equal(
		ot_res_tab_msg_alloc((otMessage *)(MSG_ADDR - CONFIG_OPENTHREAD_RPC_MESSAGE_POOL)),
		0);

	/* Verify that all allocated messages can be retrieved. */
	for (size_t i = 0; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL; i++) {
		zassert_equal(ot_res_tab_msg_get(i + 1), (otMessage *)(MSG_ADDR - i));
	}

	/* Verify that out-of-range keys return NULL. */
	zassert_is_null(ot_res_tab_msg_get(0));
	zassert_is_null(ot_res_tab_msg_get(CONFIG_OPENTHREAD_RPC_MESSAGE_POOL + 1));

	/* Free all messages and verify they can no longer be retrieved. */
	for (size_t i = 0; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL; i++) {
		ot_res_tab_msg_free(i + 1);
		zassert_is_null(ot_res_tab_msg_get(i + 1));
	}
}

/*
 * Test reception of otMessageGetOffset().
 * Test serialization of the result: 0 and 0xffff
 */
ZTEST(ot_rpc_message, test_otMessageGetOffset)
{
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageGetOffset_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_OFFSET, msg_key));

	zassert_equal(otMessageGetOffset_fake.call_count, 1);
	zassert_equal(otMessageGetOffset_fake.arg0_val, msg);

	otMessageGetOffset_fake.return_val = UINT16_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT16(UINT16_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_OFFSET, msg_key));

	zassert_equal(otMessageGetOffset_fake.call_count, 2);
	zassert_equal(otMessageGetOffset_fake.arg0_val, msg);
}

/*
 * Test reception of otMessageGetLength().
 * Test serialization of the result: 0 and 0xffff
 */
ZTEST(ot_rpc_message, test_otMessageGetLength)
{
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageGetLength_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_LENGTH, msg_key));

	zassert_equal(otMessageGetLength_fake.call_count, 1);
	zassert_equal(otMessageGetLength_fake.arg0_val, msg);

	otMessageGetLength_fake.return_val = UINT16_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT16(UINT16_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_LENGTH, msg_key));

	zassert_equal(otMessageGetLength_fake.call_count, 2);
	zassert_equal(otMessageGetLength_fake.arg0_val, msg);
}

/*
 * Test reception of otMessageGetThreadLinkInfo().
 * Test serialization of the result: OT_ERROR_NONE + link info.
 */
#define PANID		    0xabcd
#define CHANNEL		    11
#define RSS		    INT8_MIN
#define LQI		    UINT8_MAX
#define TIMESYNC_SEQ	    UINT8_MAX
#define NETWORK_TIME_OFFSET INT64_MIN
#define RADIO_TYPE	    UINT8_MAX

otError get_thread_link_info(const otMessage *msg, otThreadLinkInfo *info)
{
	info->mPanId = PANID;
	info->mChannel = CHANNEL;
	info->mRss = RSS;
	info->mLqi = LQI;
	info->mLinkSecurity = true;
	info->mIsDstPanIdBroadcast = false;
	info->mTimeSyncSeq = TIMESYNC_SEQ;
	info->mNetworkTimeOffset = NETWORK_TIME_OFFSET;
	info->mRadioType = RADIO_TYPE;

	return OT_ERROR_NONE;
}

ZTEST(ot_rpc_message, test_otMessageGetThreadLinkInfo)
{

	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageGetThreadLinkInfo_fake.custom_fake = get_thread_link_info;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, CBOR_UINT16(PANID), CHANNEL,
					   CBOR_INT8(RSS), CBOR_UINT8(LQI), CBOR_TRUE, CBOR_FALSE,
					   CBOR_UINT8(TIMESYNC_SEQ),
					   CBOR_INT64(NETWORK_TIME_OFFSET), CBOR_UINT8(RADIO_TYPE)),
				   NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_THREAD_LINK_INFO, msg_key));

	zassert_equal(otMessageGetThreadLinkInfo_fake.call_count, 1);
	zassert_equal(otMessageGetThreadLinkInfo_fake.arg0_val, msg);
}

#undef PANID
#undef CHANNEL
#undef RSS
#undef LQI
#undef TIMESYNC_SEQ
#undef NETWORK_TIME_OFFSET
#undef RADIO_TYPE

/*
 * Test reception of otMessageRead() that takes the maximum offset and length 1000.
 * Verify serialization of the result: empty byte string
 */
uint16_t message_read_empty(const otMessage *msg, uint16_t offset, void *buf, uint16_t len)
{
	return 0;
}

ZTEST(ot_rpc_message, test_otMessageRead_empty)
{
	const uint16_t offset = UINT16_MAX;
	const uint16_t len = 1024;
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageGetLength_fake.return_val = 0;
	otMessageRead_fake.custom_fake = message_read_empty;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_BSTR(0)), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_MESSAGE_READ, msg_key, CBOR_UINT16(offset), CBOR_UINT16(len)));

	zassert_equal(otMessageRead_fake.call_count, 1);
	zassert_equal(otMessageRead_fake.arg0_val, msg);
	/* Expect 0 instead of 'offset' because the code checks the actual size before read. */
	zassert_equal(otMessageRead_fake.arg1_val, 0);
	zassert_not_null(otMessageRead_fake.arg2_val);
	/* Expect 0 instead of 'len' because the code checks the actual size before read. */
	zassert_equal(otMessageRead_fake.arg3_val, 0);
}

/*
 * Test reception of otMessageRead() that takes the offset 0 and length 257.
 * Verify serialization of the result: 256B string
 */
uint16_t message_read_long(const otMessage *msg, uint16_t offset, void *buf, uint16_t len)
{
	memset(buf, 'a', len);

	return len;
}

ZTEST(ot_rpc_message, test_otMessageRead_long)
{
	const uint16_t offset = 1;
	const uint16_t len = UINT16_MAX;
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageGetLength_fake.return_val = offset + LONG_BSTR_LEN;
	otMessageRead_fake.custom_fake = message_read_long;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_BSTR16(LONG_BSTR_LEN, STR_SEQUENCE(LONG_BSTR_LEN))),
				   NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_MESSAGE_READ, msg_key, CBOR_UINT16(offset), CBOR_UINT16(len)));

	zassert_equal(otMessageRead_fake.call_count, 1);
	zassert_equal(otMessageRead_fake.arg0_val, msg);
	zassert_equal(otMessageRead_fake.arg1_val, offset);
	zassert_not_null(otMessageRead_fake.arg2_val);
	/* Expect this instead of 'len' because the code checks the actual size before read. */
	zassert_equal(otMessageRead_fake.arg3_val, LONG_BSTR_LEN);
}

/*
 * Test reception of otMessageAppend() that takes 1 byte.
 * Test serialization of the result: OT_ERROR_NONE
 */
otError message_append_small(otMessage *msg, const void *buf, uint16_t len)
{
	zassert_equal(len, 1);
	zassert_equal(*((const uint8_t *)buf), 's');

	return OT_ERROR_NONE;
}

ZTEST(ot_rpc_message, test_otMessageAppend_small)
{
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageAppend_fake.custom_fake = message_append_small;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_APPEND, msg_key, CBOR_BSTR(1, 's')));

	zassert_equal(otMessageAppend_fake.call_count, 1);
	zassert_equal(otMessageAppend_fake.arg0_val, msg);
}

/*
 * Test reception of otMessageAppend() that takes 256 bytes.
 * Test serialization of the result: OT_ERROR_NO_BUFS
 */
otError message_append_big(otMessage *msg, const void *buf, uint16_t len)
{
	const static uint8_t exp[] = {STR_SEQUENCE(LONG_BSTR_LEN)};

	zassert_equal(len, LONG_BSTR_LEN);
	zassert_mem_equal(buf, exp, LONG_BSTR_LEN);

	return OT_ERROR_NO_BUFS;
}

ZTEST(ot_rpc_message, test_otMessageAppend_big)
{
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	otMessageAppend_fake.custom_fake = message_append_big;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NO_BUFS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_APPEND, msg_key,
					CBOR_BSTR16(LONG_BSTR_LEN, STR_SEQUENCE(LONG_BSTR_LEN))));

	zassert_equal(otMessageAppend_fake.call_count, 1);
	zassert_equal(otMessageAppend_fake.arg0_val, msg);
}

/*
 * Test reception of otMessageFree().
 * Verify that the message is removed from the message table.
 */
ZTEST(ot_rpc_message, test_otMessageFree)
{
	otMessage *msg = (otMessage *)MSG_ADDR;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_FREE, msg_key));

	zassert_equal(otMessageFree_fake.call_count, 1);
	zassert_equal(otMessageFree_fake.arg0_val, msg);
	zassert_is_null(ot_res_tab_msg_get(msg_key));
}

/*
 * Test reception of otMessageRegisterTxCallback() with non-null callback.
 * Test serialization of otMessageTxCallback.
 * Test reception of otMessageRegisterTxCallback() with null callback.
 */
ZTEST(ot_rpc_message, test_otMessageRegisterTxCallback)
{
	otMessage *msg = (otMessage *)MSG_ADDR;
	void *ctx = (void *)MSG_TX_CB_ID;
	ot_rpc_res_tab_key msg_key = ot_res_tab_msg_alloc(msg);

	/* Test reception of otMessageRegisterTxCallback with non-null callback. */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_REGISTER_TX_CALLBACK, msg_key,
					CBOR_UINT32(MSG_TX_CB_ID)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMessageRegisterTxCallback_fake.call_count, 1);
	zassert_equal(otMessageRegisterTxCallback_fake.arg0_val, msg);
	zassert_not_null(otMessageRegisterTxCallback_fake.arg1_val);
	zassert_equal(otMessageRegisterTxCallback_fake.arg2_val, ctx);

	/* Test serialization of otMessageTxCallback.
	 * Note that the callback allocates a new message key.
	 */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_MESSAGE_TX_CB, CBOR_UINT32(MSG_TX_CB_ID),
					   msg_key + 1, OT_ERROR_NO_ACK),
				   RPC_RSP());
	ot_rpc_mutex_lock();
	otMessageRegisterTxCallback_fake.arg1_val(msg, OT_ERROR_NO_ACK, ctx);
	ot_rpc_mutex_unlock();
	mock_nrf_rpc_tr_expect_done();

	/* Test reception of otMessageRegisterTxCallback with null callback. */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_REGISTER_TX_CALLBACK, msg_key, 0));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMessageRegisterTxCallback_fake.call_count, 2);
	zassert_equal(otMessageRegisterTxCallback_fake.arg0_val, msg);
	zassert_is_null(otMessageRegisterTxCallback_fake.arg1_val);
}

ZTEST_SUITE(ot_rpc_message, NULL, NULL, tc_setup, tc_cleanup, NULL);
