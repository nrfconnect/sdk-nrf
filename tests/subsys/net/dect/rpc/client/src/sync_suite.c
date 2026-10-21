/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <string.h>

#include <zephyr/kernel.h>
#include "dect_rpc_client_net.h"
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static struct net_if *sync_event_iface;
static atomic_t sync_addr_event_count;

static void sync_addr_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				    struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (iface == sync_event_iface &&
	    (mgmt_event == NET_EVENT_IPV6_ADDR_ADD || mgmt_event == NET_EVENT_IPV6_ADDR_DEL)) {
		atomic_inc(&sync_addr_event_count);
	}
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_client_sync, test_dect_sync_shell)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int err;

	shell_backend_dummy_clear_output(sh);

	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(1)),
				   RPC_RSP(GET_ADDRS_RSP_BOOTED_UNASSOCIATED));
	err = shell_execute_cmd(sh, "dect sync");
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "dect sync failed: %d", err);
	output = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(output, "Synced addresses/prefixes from server"),
			 "Missing sync success: %s", output);
}

ZTEST(dect_rpc_client_sync, test_sync_addrs_api_applies_mtu)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	int err;

	zassert_not_null(iface, "DECT RPC client iface missing");

	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(2)),
				   RPC_RSP(GET_ADDRS_RSP_BOOTED_UNASSOCIATED));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "sync addrs failed: %d", err);
	zassert_equal(net_if_get_mtu(iface), 1280, "MTU should mirror GET_ADDRS response");
}

#define ONE_ADDR_BYTES                                                                             \
	0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, \
		0x0f

#define ONE_PREFIX_BYTES                                                                           \
	0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		0x01

static int count_ipv6_prefixes(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6;
	int n = 0;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return -1;
	}

	for (int i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (ipv6->prefix[i].is_used) {
			n++;
		}
	}

	return n;
}

static const struct net_in6_addr *first_ipv6_prefix(struct net_if *iface, uint8_t *len_out)
{
	struct net_if_ipv6 *ipv6;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return NULL;
	}

	for (int i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (ipv6->prefix[i].is_used) {
			if (len_out) {
				*len_out = ipv6->prefix[i].len;
			}
			return &ipv6->prefix[i].prefix;
		}
	}

	return NULL;
}

ZTEST(dect_rpc_client_sync, test_sync_addrs_applies_one_addr)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	struct net_if_ipv6 *ipv6;
	int err;

	zassert_not_null(iface, "DECT RPC client iface missing");

	mock_dect_rpc_tr_expect_add(
		RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(3)),
		RPC_RSP(CBOR_UINT_SMALL(1), CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_UINT_SMALL(0),
			CBOR_UINT16(1280), CBOR_FALSE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "sync addrs failed: %d", err);
	zassert_ok(net_if_config_ipv6_get(iface, &ipv6));
	zassert_true(ipv6->unicast[0].is_used, "expected one unicast addr applied");
}

ZTEST(dect_rpc_client_sync, test_sync_addrs_applies_one_prefix)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	const struct net_in6_addr *prefix;
	uint8_t prefix_len = 0;
	int err;
	static const uint8_t expected_prefix[] = { ONE_PREFIX_BYTES };

	zassert_not_null(iface, "DECT RPC client iface missing");

	mock_dect_rpc_tr_expect_add(
		RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(4)),
		RPC_RSP(CBOR_UINT_SMALL(0), CBOR_UINT_SMALL(1), CBOR_BSTR(16, ONE_PREFIX_BYTES),
			CBOR_UINT8(64), CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "sync addrs failed: %d", err);
	zassert_equal(count_ipv6_prefixes(iface), 1, "expected one prefix applied");
	prefix = first_ipv6_prefix(iface, &prefix_len);
	zassert_not_null(prefix, "prefix missing on iface");
	zassert_equal(prefix_len, 64, "prefix length");
	zassert_mem_equal(prefix->s6_addr, expected_prefix, sizeof(expected_prefix),
			  "prefix bytes");
}

ZTEST(dect_rpc_client_sync, test_sync_addrs_skips_prefix_len_over_128)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	int err;

	zassert_not_null(iface, "DECT RPC client iface missing");

	mock_dect_rpc_tr_expect_add(
		RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(7)),
		RPC_RSP(CBOR_UINT_SMALL(0), CBOR_UINT_SMALL(1), CBOR_BSTR(16, ONE_PREFIX_BYTES),
			CBOR_UINT8(129), CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "sync addrs failed: %d", err);
	zassert_equal(count_ipv6_prefixes(iface), 0, "invalid prefix len must not be applied");
}

ZTEST(dect_rpc_client_sync, test_sync_addrs_skips_extra_wire_addrs)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	int err;

	zassert_not_null(iface, "DECT RPC client iface missing");

	/* Wire claims 9 unicast addrs; client applies 8, then skips extras on decode. */
	mock_dect_rpc_tr_expect_add(
		RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(5)),
		RPC_RSP(CBOR_UINT_SMALL(9),
			CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_BSTR(16, ONE_ADDR_BYTES),
			CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_BSTR(16, ONE_ADDR_BYTES),
			CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_BSTR(16, ONE_ADDR_BYTES),
			CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_BSTR(16, ONE_ADDR_BYTES),
			CBOR_BSTR(16, ONE_ADDR_BYTES),
			CBOR_UINT_SMALL(0), CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "sync addrs failed: %d", err);
}

ZTEST(dect_rpc_client_sync, test_sync_addrs_skips_extra_wire_prefixes)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	int err;

	zassert_not_null(iface, "DECT RPC client iface missing");

	/* Wire claims 5 prefixes; client applies 4, then skips extras on decode. */
	mock_dect_rpc_tr_expect_add(
		RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS, CBOR_UINT_SMALL(6)),
		RPC_RSP(CBOR_UINT_SMALL(0), CBOR_UINT_SMALL(5),
			CBOR_BSTR(16, ONE_PREFIX_BYTES), CBOR_UINT8(64),
			CBOR_BSTR(16, ONE_PREFIX_BYTES), CBOR_UINT8(64),
			CBOR_BSTR(16, ONE_PREFIX_BYTES), CBOR_UINT8(64),
			CBOR_BSTR(16, ONE_PREFIX_BYTES), CBOR_UINT8(64),
			CBOR_BSTR(16, ONE_PREFIX_BYTES), CBOR_UINT8(64),
			CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "sync addrs failed: %d", err);
}

ZTEST(dect_rpc_client_sync, test_z_sync_addrs_is_idempotent_and_rejects_malformed_response)
{
	static const uint8_t expected_addr[] = { ONE_ADDR_BYTES };
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	struct net_if_ipv6 *ipv6;
	struct net_mgmt_event_callback event_cb;
	int err;

	zassert_not_null(iface, "DECT RPC client iface missing");
	sync_event_iface = iface;
	atomic_set(&sync_addr_event_count, 0);
	net_mgmt_init_event_callback(&event_cb, sync_addr_event_handler,
				    NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL);
	net_mgmt_add_event_callback(&event_cb);

	/* First sync establishes the mirrored address; a repeated response must be a no-op. */
	mock_dect_rpc_tr_expect_add(
		(mock_dect_rpc_pkt_t){ .data = NULL, .len = 0 },
		RPC_RSP(CBOR_UINT_SMALL(1), CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_UINT_SMALL(0),
			CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();
	zassert_equal(err, 0, "initial sync failed: %d", err);
	k_msleep(50);

	atomic_set(&sync_addr_event_count, 0);
	mock_dect_rpc_tr_expect_add(
		(mock_dect_rpc_pkt_t){ .data = NULL, .len = 0 },
		RPC_RSP(CBOR_UINT_SMALL(1), CBOR_BSTR(16, ONE_ADDR_BYTES), CBOR_UINT_SMALL(0),
			CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();
	zassert_equal(err, 0, "repeated sync failed: %d", err);
	k_msleep(50);
	zassert_equal(atomic_get(&sync_addr_event_count), 0,
		      "unchanged sync must not emit IPv6 address events");

	/* A short address payload makes the response invalid; retain the last good address. */
	mock_dect_rpc_tr_expect_add(
		(mock_dect_rpc_pkt_t){ .data = NULL, .len = 0 },
		RPC_RSP(CBOR_UINT_SMALL(1), CBOR_BSTR(15, 0x20, 0x01, 0x02, 0x03, 0x04, 0x05,
					     0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
					     0x0d, 0x0e),
			CBOR_UINT_SMALL(0), CBOR_UINT16(1280), CBOR_TRUE, CBOR_TRUE));
	err = dect_rpc_client_sync_addrs(iface);
	mock_dect_rpc_tr_expect_done();
	zassert_equal(err, -EIO, "malformed sync should fail: %d", err);
	zassert_ok(net_if_config_ipv6_get(iface, &ipv6));
	zassert_true(ipv6->unicast[0].is_used, "malformed sync removed the previous address");
	zassert_mem_equal(ipv6->unicast[0].address.in6_addr.s6_addr,
			  expected_addr, sizeof(expected_addr),
			  "malformed sync changed the previous address");

	net_mgmt_del_event_callback(&event_cb);
	sync_event_iface = NULL;
}

ZTEST_SUITE(dect_rpc_client_sync, NULL, NULL, tc_setup, NULL, NULL);
