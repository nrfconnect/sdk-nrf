/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/ztest.h>

#include "dect_rpc_client_net.h"

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_client_events, test_if_receive_event)
{
	static const uint8_t mini_pkt[8] = { 0x60, 0, 0, 0, 0, 0, 0, 0x3b };

	mock_dect_rpc_tr_expect_add(RPC_EVT_ACK(DECT_RPC_CMD_IF_RECEIVE), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_EVT(DECT_RPC_CMD_IF_RECEIVE, CBOR_BSTR(8, mini_pkt[0],
									   mini_pkt[1],
									   mini_pkt[2],
									   mini_pkt[3],
									   mini_pkt[4],
									   mini_pkt[5],
									   mini_pkt[6],
									   mini_pkt[7])));
	k_msleep(50);
	mock_dect_rpc_tr_expect_done();
}

ZTEST(dect_rpc_client_events, test_shell_line_event)
{
	mock_dect_rpc_tr_expect_add(RPC_EVT_ACK(DECT_RPC_CMD_SHELL_LINE), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_EVT(DECT_RPC_CMD_SHELL_LINE,
					CBOR_BSTR(12, 'l', 'i', 'n', 'e', ' ', 'o', 'u', 't',
						  'p', 'u', 't', '\n')));
	k_msleep(50);
	mock_dect_rpc_tr_expect_done();
}

ZTEST(dect_rpc_client_events, test_if_link_state_event)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));

	zassert_not_null(iface, "DECT RPC client iface missing");

	/* Carrier off avoids IPv6 MLD join RPC side traffic on carrier-on. */
	mock_dect_rpc_tr_expect_add(RPC_EVT_ACK(DECT_RPC_CMD_IF_LINK_STATE), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_EVT(DECT_RPC_CMD_IF_LINK_STATE, CBOR_FALSE, CBOR_TRUE));
	k_msleep(50);
	mock_dect_rpc_tr_expect_done();

	zassert_false(net_if_is_carrier_ok(iface), "carrier should mirror server event");
	zassert_true(net_if_is_dormant(iface), "dormant should mirror server event");
}

ZTEST(dect_rpc_client_events, test_if_addrs_changed_triggers_get_addrs)
{
	mock_dect_rpc_tr_expect_add(RPC_EVT_ACK(DECT_RPC_CMD_IF_ADDRS_CHANGED), NO_RSP);
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(0)),
				   RPC_RSP(GET_ADDRS_RSP_BOOTED_UNASSOCIATED));
	mock_dect_rpc_tr_receive(RPC_EVT(DECT_RPC_CMD_IF_ADDRS_CHANGED, CBOR_UINT_SMALL(0)));
	k_msleep(100);
	mock_dect_rpc_tr_expect_done();
}

ZTEST_SUITE(dect_rpc_client_events, NULL, NULL, tc_setup, NULL, NULL);
