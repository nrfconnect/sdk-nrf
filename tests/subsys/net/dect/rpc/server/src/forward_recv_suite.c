/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Exercises dect_rpc_server_forward_recv() -> evt msg queue (401-403) and
 * dect_rpc_evt_work_fn() IF_RECEIVE CBOR (175-189), including L2 recv when the
 * RPC client session is active (dect_net_l2.c -> dect_rpc_server_forward_recv).
 */

#include "dect_rpc_common.h"
#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
#include "server_dect_fixture.h"

#include <net/dect/dect_net_l2_rpc.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/ztest.h>

void dect_rpc_server_forward_recv(struct net_if *iface, struct net_pkt *pkt);
void dect_rpc_server_notify_addrs_changed(void);

static struct net_if *dect_iface;

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *suite_setup(void)
{
	dect_iface = server_dect_fixture_ensure_booted();
	zassert_not_null(dect_iface, "DECT stack did not boot");
	return NULL;
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

/*
 * net_pkt_write() advances the read cursor; L2-delivered packets start at offset 0.
 * forward_recv() uses net_pkt_read() without rewinding.
 */
static struct net_pkt *pkt_with_payload(const uint8_t *data, size_t len)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(dect_iface, len, AF_INET6, 0, K_NO_WAIT);
	zassert_not_null(pkt, "net_pkt alloc failed");
	zassert_ok(net_pkt_write(pkt, data, len));
	net_pkt_cursor_init(pkt);
	return pkt;
}

static mock_dect_rpc_pkt_t if_receive_evt_bstr(const uint8_t *bstr, size_t len)
{
	static uint8_t wire[128];
	size_t off = 0;

	wire[off++] = 0x00;
	wire[off++] = DECT_RPC_CMD_IF_RECEIVE;
	wire[off++] = 0xff;
	wire[off++] = 0x00;
	wire[off++] = 0x00;

	if (len <= 23U) {
		wire[off++] = (uint8_t)(0x40U | len);
	} else {
		wire[off++] = 0x58;
		wire[off++] = (uint8_t)len;
	}
	memcpy(wire + off, bstr, len);
	off += len;
	wire[off++] = CBOR_NULL;

	return (mock_dect_rpc_pkt_t){ .data = wire, .len = off };
}

static void wait_for_mock_send_count(size_t expected)
{
	for (int i = 0; i < 30 && mock_dect_rpc_tr_sent_count() < expected; i++) {
		k_msleep(10);
	}
	zassert_equal(mock_dect_rpc_tr_sent_count(), expected, "mock send count");
}

ZTEST(dect_rpc_server_forward_recv, test_forward_recv_sends_if_receive_evt)
{
	static const uint8_t mini_pkt[8] = { 0x60, 0, 0, 0, 0, 0, 0, 0x3b };
	struct net_pkt *pkt;

	zassert_true(NRF_RPC_GROUP_STATUS(dect_rpc_group),
		     "RPC group must be bound (see tc_setup nrf_rpc_init)");

	mock_dect_rpc_tr_expect_add(
		RPC_EVT(DECT_RPC_CMD_IF_RECEIVE, CBOR_BSTR(8, mini_pkt[0], mini_pkt[1], mini_pkt[2],
							  mini_pkt[3], mini_pkt[4], mini_pkt[5],
							  mini_pkt[6], mini_pkt[7])),
		NO_RSP);

	pkt = pkt_with_payload(mini_pkt, sizeof(mini_pkt));
	dect_rpc_server_forward_recv(dect_iface, pkt);
	net_pkt_unref(pkt);

	k_msleep(100);
	zassert_equal(mock_dect_rpc_tr_sent_count(), 1U, "expected one IF_RECEIVE event on wire");
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_server_forward_recv, test_forward_recv_longer_payload_bstr_on_wire)
{
	static const uint8_t payload[24] = {
		0x60, 0, 0, 0, 0, 0, 0, 0x3b, 0x11, 0x22, 0x33, 0x44,
		0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01,
	};
	struct net_pkt *pkt;

	zassert_true(NRF_RPC_GROUP_STATUS(dect_rpc_group), "RPC group bound");

	mock_dect_rpc_tr_expect_add(
		RPC_EVT(DECT_RPC_CMD_IF_RECEIVE,
			CBOR_BSTR8(24, payload[0], payload[1], payload[2], payload[3], payload[4],
				   payload[5], payload[6], payload[7], payload[8], payload[9],
				   payload[10], payload[11], payload[12], payload[13], payload[14],
				   payload[15], payload[16], payload[17], payload[18], payload[19],
				   payload[20], payload[21], payload[22], payload[23])),
		NO_RSP);

	pkt = pkt_with_payload(payload, sizeof(payload));
	dect_rpc_server_forward_recv(dect_iface, pkt);
	net_pkt_unref(pkt);

	k_msleep(100);
	zassert_equal(mock_dect_rpc_tr_sent_count(), 1U, "expected IF_RECEIVE with 24-byte bstr");
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_server_forward_recv, test_addrs_changed_survives_full_event_queue)
{
	static const uint8_t mini_pkt[8] = { 0x60, 0, 0, 0, 0, 0, 0, 0x3b };
	static const uint8_t expected_if_receive[] = {
		0x00, DECT_RPC_CMD_IF_RECEIVE, 0xff, 0x00, 0x00,
		CBOR_BSTR(8, 0x60, 0, 0, 0, 0, 0, 0, 0x3b), CBOR_NULL,
	};
	static const uint8_t expected_if_addrs_changed[] = {
		0x00, DECT_RPC_CMD_IF_ADDRS_CHANGED, 0xff, 0x00, 0x00, CBOR_UINT_SMALL(0),
		CBOR_NULL,
	};
	const mock_dect_rpc_pkt_t receive_expect = {
		.data = expected_if_receive,
		.len = sizeof(expected_if_receive),
	};
	const mock_dect_rpc_pkt_t addrs_changed_expect = {
		.data = expected_if_addrs_changed,
		.len = sizeof(expected_if_addrs_changed),
	};
	const size_t queue_len = CONFIG_DECT_NR_RPC_SERVER_EVT_QUEUE_LEN;
	struct net_pkt *pkt;

	zassert_true(NRF_RPC_GROUP_STATUS(dect_rpc_group), "RPC group must be bound");
	mock_dect_rpc_tr_expect_add(receive_expect, NO_RSP);
	mock_dect_rpc_tr_expect_add(addrs_changed_expect, NO_RSP);
	for (size_t i = 1; i < queue_len; i++) {
		mock_dect_rpc_tr_expect_add(receive_expect, NO_RSP);
	}

	/* Keep the worker from draining the bounded queue until it is full. */
	k_sched_lock();
	for (size_t i = 0; i < queue_len; i++) {
		pkt = pkt_with_payload(mini_pkt, sizeof(mini_pkt));
		dect_rpc_server_forward_recv(dect_iface, pkt);
		net_pkt_unref(pkt);
	}
	dect_rpc_server_notify_addrs_changed();
	k_sched_unlock();

	wait_for_mock_send_count(queue_len + 1);
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_server_forward_recv, test_l2_net_recv_forwards_to_rpc_client)
{
	static const uint8_t l2_ipv6_frame[40] = {
		0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
	};
	const struct net_l2 *l2 = net_if_l2(dect_iface);
	struct net_pkt *pkt;
	enum net_verdict verdict;

	zassert_true(NRF_RPC_GROUP_STATUS(dect_rpc_group), "RPC group bound");
	zassert_not_null(l2);
	zassert_not_null(l2->recv, "DECT L2 recv handler missing");

	dect_net_l2_rpc_client_set_connected(true);

	mock_dect_rpc_tr_expect_add(if_receive_evt_bstr(l2_ipv6_frame, sizeof(l2_ipv6_frame)),
				   NO_RSP);

	pkt = pkt_with_payload(l2_ipv6_frame, sizeof(l2_ipv6_frame));
	verdict = l2->recv(dect_iface, pkt);
	zassert_equal(verdict, NET_OK, "RPC session should consume pkt in L2 (forward to client)");

	wait_for_mock_send_count(1U);
	mock_dect_rpc_tr_expect_reset();
	dect_net_l2_rpc_client_set_connected(false);
}

ZTEST(dect_rpc_server_forward_recv, test_forward_recv_null_and_empty)
{
	zassert_equal(mock_dect_rpc_tr_sent_count(), 0U, "no RPC before test");

	dect_rpc_server_forward_recv(dect_iface, NULL);

	struct net_pkt *pkt = net_pkt_alloc_with_buffer(dect_iface, 4, AF_INET6, 0, K_NO_WAIT);

	zassert_not_null(pkt, "net_pkt alloc failed");
	dect_rpc_server_forward_recv(dect_iface, pkt);
	net_pkt_unref(pkt);

	k_msleep(50);
	zassert_equal(mock_dect_rpc_tr_sent_count(), 0U, "null/empty pkt must not emit IF_RECEIVE");
}

ZTEST_SUITE(dect_rpc_server_forward_recv, NULL, suite_setup, tc_setup, NULL, NULL);
