/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common_fakes.h"

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_resource.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/message.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

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
	f(otUdpNewMessage);

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

ZTEST_SUITE(ot_rpc_message, NULL, NULL, tc_setup, tc_cleanup, NULL);
