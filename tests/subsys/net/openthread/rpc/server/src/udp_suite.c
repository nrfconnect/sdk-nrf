/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/udp.h>

#define FOREACH_FAKE(f)                                                                            \
	f(otUdpIsOpen);                                                                            \
	f(otUdpOpen);                                                                              \
	f(otUdpClose);                                                                             \
	f(otUdpBind);                                                                              \
	f(otUdpConnect);                                                                           \
	f(otUdpSend)

FAKE_VALUE_FUNC(bool, otUdpIsOpen, otInstance *, const otUdpSocket *);
FAKE_VALUE_FUNC(otError, otUdpOpen, otInstance *, otUdpSocket *, otUdpReceive, void *);
FAKE_VALUE_FUNC(otError, otUdpClose, otInstance *, otUdpSocket *);
FAKE_VALUE_FUNC(otError, otUdpBind, otInstance *, otUdpSocket *, const otSockAddr *,
		otNetifIdentifier);
FAKE_VALUE_FUNC(otError, otUdpConnect, otInstance *, otUdpSocket *, const otSockAddr *);
FAKE_VALUE_FUNC(otError, otUdpSend, otInstance *, otUdpSocket *, otMessage *,
		const otMessageInfo *);

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
}

/*
 * Test reception of otUdpIsOpen() command with non-existent socket key.
 * Test serialization of the result: false.
 */
ZTEST(ot_rpc_udp, test_otUdpIsOpen_false)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_FALSE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_IS_OPEN, CBOR_UINT32(1)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otUdpIsOpen_fake.call_count, 0);
}

ZTEST_SUITE(ot_rpc_udp, NULL, NULL, tc_setup, NULL, NULL);
