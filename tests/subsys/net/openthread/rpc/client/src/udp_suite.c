/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "openthread/ip6.h"
#include "openthread/message.h"
#include <mock_nrf_rpc_transport.h>
#include <openthread/error.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/udp.h>

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

static void tc_clean(void *f)
{
	mock_nrf_rpc_tr_expect_reset();
}

ZTEST(ot_rpc_udp, test_otUdpOpen_correct)
{
	otError error;
	otUdpSocket socket;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_UDP_OPEN, CBOR_UINT32((ot_socket_key)&socket)),
		RPC_RSP(OT_ERROR_NONE));
	error = otUdpOpen(NULL, &socket, NULL, NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

ZTEST(ot_rpc_udp, test_otUdpOpen_invalid)
{
	otError error;

	error = otUdpOpen(NULL, NULL, NULL, NULL);

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

ZTEST(ot_rpc_udp, test_otUdpClose_correct)
{
	otError error;
	otUdpSocket socket;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_UDP_CLOSE, CBOR_UINT32((ot_socket_key)&socket)),
		RPC_RSP(OT_ERROR_NONE));
	error = otUdpClose(NULL, &socket);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

ZTEST(ot_rpc_udp, test_otUdpClose_invalid)
{
	otError error;

	error = otUdpClose(NULL, NULL);

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

ZTEST(ot_rpc_udp, test_otUdpBind_invalid)
{
	otError error = OT_ERROR_NONE;
	otUdpSocket socket;
	otNetifIdentifier net_if = OT_NETIF_THREAD;
	otSockAddr sock_addr = {.mAddress.mFields.m8 = {ADDR_1}, .mPort = 1024};

	error = otUdpBind(NULL, &socket, NULL, net_if);

	zassert_equal(error, OT_ERROR_INVALID_ARGS);

	error = otUdpBind(NULL, NULL, &sock_addr, net_if);

	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

ZTEST(ot_rpc_udp, test_otUdpBind_correct)
{
	otError error = OT_ERROR_NONE;
	otUdpSocket socket;
	otNetifIdentifier net_if = OT_NETIF_THREAD;
	otSockAddr sock_addr = {.mAddress.mFields.m8 = {ADDR_1}, .mPort = 1024};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_UDP_BIND, CBOR_UINT32((ot_socket_key)&socket),
					   CBOR_SOC_ADDR, OT_NETIF_THREAD),
				   RPC_RSP(OT_ERROR_NONE));
	error = otUdpBind(NULL, &socket, &sock_addr, net_if);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

ZTEST(ot_rpc_udp, test_otUdpConnect_correct)
{
	otError error;
	otUdpSocket socket;
	otSockAddr sock_addr = {.mAddress.mFields.m8 = {ADDR_1}, .mPort = 1024};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_UDP_CONNECT, CBOR_UINT32((ot_socket_key)&socket), CBOR_SOC_ADDR),
		RPC_RSP(OT_ERROR_NONE));
	error = otUdpConnect(NULL, &socket, &sock_addr);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

ZTEST(ot_rpc_udp, test_otUdpConnect_invalid)
{
	otError error;
	otUdpSocket socket;
	otSockAddr sock_addr = {.mAddress.mFields.m8 = {ADDR_1}, .mPort = 1024};

	error = otUdpConnect(NULL, &socket, NULL);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);

	error = otUdpConnect(NULL, NULL, &sock_addr);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

ZTEST(ot_rpc_udp, test_otUdpSend_correct)
{
	otError error;
	otUdpSocket socket;
	otMessage *msg = (otMessage *)1;
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

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_UDP_SEND, CBOR_UINT32((ot_socket_key)&socket), 1, CBOR_MSG_INFO),
		RPC_RSP(OT_ERROR_NONE));
	error = otUdpSend(NULL, &socket, msg, &message_info);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

ZTEST(ot_rpc_udp, test_otUdpSend_invalid)
{
	otError error;
	otUdpSocket socket;
	otMessage *msg = (otMessage *)1;
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

	error = otUdpSend(NULL, NULL, msg, &message_info);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);

	error = otUdpSend(NULL, &socket, NULL, &message_info);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);

	error = otUdpSend(NULL, &socket, msg, NULL);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

FAKE_VOID_FUNC(handle_udp_receive, void *, otMessage *, const otMessageInfo *);

ZTEST(ot_rpc_udp, test_udp_receive)
{
	otError error;
	otSockAddr listen_sock_addr = {.mAddress.mFields.m8 = {ADDR_1}, .mPort = 1024};
	otUdpSocket udp_socket;

	listen_sock_addr.mPort = 1024;
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_UDP_OPEN, CBOR_UINT32((ot_socket_key)&udp_socket)),
		RPC_RSP(OT_ERROR_NONE));
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_UDP_BIND,
					   CBOR_UINT32((ot_socket_key)&udp_socket), CBOR_SOC_ADDR,
					   OT_NETIF_THREAD),
				   RPC_RSP(OT_ERROR_NONE));
	error = otUdpOpen(NULL, &udp_socket, handle_udp_receive, NULL);
	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal(udp_socket.mHandler, handle_udp_receive);

	error = otUdpBind(NULL, &udp_socket, &listen_sock_addr, OT_NETIF_THREAD);
	zassert_equal(error, OT_ERROR_NONE);
	mock_nrf_rpc_tr_expect_done();

	RESET_FAKE(handle_udp_receive);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_UDP_RECEIVE_CB,
					CBOR_UINT32((ot_socket_key)&udp_socket), 1, CBOR_MSG_INFO));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(handle_udp_receive_fake.call_count, 1);
	zassert_equal(handle_udp_receive_fake.arg0_val, (void *)NULL);
	zassert_equal(handle_udp_receive_fake.arg1_val, (otMessage *)1);
}

ZTEST_SUITE(ot_rpc_udp, NULL, NULL, tc_setup, tc_clean, NULL);
