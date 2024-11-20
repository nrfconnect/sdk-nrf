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

#include <openthread/message.h>
#include "common_fakes.h"
#include "zephyr/ztest_assert.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

#define FOREACH_FAKE(f)                                                                            \
	f(otMessageGetLength);                                                                     \
	f(otMessageGetOffset);                                                                     \
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

ZTEST(ot_rpc_message, test_otUdpNewMessage_failing)
{
	const uint8_t prio = 40;

	otUdpNewMessage_fake.return_val = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_NULL));

	zassert_is_null(otUdpNewMessage_fake.arg1_val);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_TRUE, CBOR_UINT8(prio)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otUdpNewMessage_fake.call_count, 2);
	zassert_true(otUdpNewMessage_fake.arg1_val->mLinkSecurityEnabled);
	zassert_equal(otUdpNewMessage_fake.arg1_val->mPriority, prio);
}

ZTEST(ot_rpc_message, test_otUdpNewMessage_free_working)
{
	const uint8_t prio = 40;

	otUdpNewMessage_fake.return_val = (otMessage *)1;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(1), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_NULL));

	zassert_is_null(otUdpNewMessage_fake.arg1_val);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(2), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_NEW_MESSAGE, CBOR_TRUE, CBOR_UINT8(prio)));

	zassert_equal(otUdpNewMessage_fake.call_count, 2);
	zassert_true(otUdpNewMessage_fake.arg1_val->mLinkSecurityEnabled);
	zassert_equal(otUdpNewMessage_fake.arg1_val->mPriority, prio);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_FREE, 1));

	zassert_equal(otMessageFree_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_MESSAGE_FREE, 2));

	zassert_equal(otMessageFree_fake.arg0_val, (otMessage *)1);

	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otMessageFree_fake.call_count, 2);
}

ZTEST(ot_rpc_message, test_ot_reg_msg_alloc_free)
{

	for (size_t i = 1; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL + 1; i++) {
		zassert_equal(ot_reg_msg_alloc((otMessage *)i), i);
	}

	zassert_equal(ot_reg_msg_alloc((otMessage *)UINT32_MAX), 0);

	for (size_t i = 1; i < CONFIG_OPENTHREAD_RPC_MESSAGE_POOL + 1; i++) {
		ot_msg_free(i);
	}
}

ZTEST(ot_rpc_message, test_ot_reg_msg_get)
{

	zassert_equal(ot_reg_msg_alloc((otMessage *)1), 1);

	zassert_equal(ot_msg_get(1), (otMessage *)1);

	zassert_equal(ot_msg_get(CONFIG_OPENTHREAD_RPC_MESSAGE_POOL), NULL);

	ot_msg_free(1);
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

ZTEST_SUITE(ot_rpc_message, NULL, NULL, tc_setup, NULL, NULL);
