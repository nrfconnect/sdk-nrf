/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_lock.h>
#include <ot_rpc_resource.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/message.h>
#include "common_fakes.h"
#include "zephyr/ztest_assert.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

/* Message TX callback ID */
#define MSG_TX_CB_ID (UINT32_MAX - 1)

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

ZTEST(ot_rpc_message, test_ot_res_tab_msg_alloc_free)
{

	for (size_t i = 1; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL + 1; i++) {
		zassert_equal(ot_res_tab_msg_alloc((otMessage *)i), i);
	}

	zassert_equal(ot_res_tab_msg_alloc((otMessage *)UINT32_MAX), 0);

	for (size_t i = 1; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL + 1; i++) {
		ot_res_tab_msg_free(i);
	}
}

ZTEST(ot_rpc_message, test_ot_res_tab_msg_get)
{

	zassert_equal(ot_res_tab_msg_alloc((otMessage *)1), 1);

	zassert_equal(ot_res_tab_msg_get(1), (otMessage *)1);

	zassert_equal(ot_res_tab_msg_get(CONFIG_OPENTHREAD_RPC_MESSAGE_POOL), NULL);

	ot_res_tab_msg_free(1);
}

ZTEST(ot_rpc_message, test_get_length_offset)
{
	otUdpNewMessage_fake.return_val = (otMessage *)1;
	otMessageGetLength_fake.return_val = 1;
	otMessageGetOffset_fake.return_val = 1;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(1), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_NULL));

	zassert_equal(otUdpNewMessage_fake.call_count, 1);
	zassert_is_null(otUdpNewMessage_fake.arg1_val);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(1), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_LENGTH, 1));

	zassert_equal(otMessageGetLength_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(1), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_OFFSET, 1));

	zassert_equal(otMessageGetOffset_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_GET_LENGTH, 0));

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_FREE, 1));

	zassert_equal(otMessageFree_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMessageGetLength_fake.call_count, 1);
	zassert_equal(otMessageFree_fake.call_count, 1);
}

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

	/* Cleanup */
	ot_res_tab_msg_free(msg_key);
}

ZTEST_SUITE(ot_rpc_message, NULL, NULL, tc_setup, NULL, NULL);
