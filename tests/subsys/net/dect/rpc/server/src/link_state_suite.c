/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Tests that real l2_dect association state changes genuinely drive the DECT NR+ RPC
 * server's registered hooks (dect_net_l2_link_state_register() ->
 * dect_rpc_server_link_state_changed()) -- not just that the hook function works when
 * called directly. server_dect_fixture_associate()/_release() drive a real network scan
 * via server_dect_fixture (L2 parent association create/remove), so this exercises the
 * actual hook wiring inside
 * subsys/net/dect/l2/dect_net_l2.c end to end.
 *
 * Event delivery (DECT_RPC_CMD_IF_LINK_STATE / DECT_RPC_CMD_IF_ADDRS_CHANGED) happens
 * asynchronously on the system workqueue (dect_rpc_evt_work in server/dect_rpc_if.c).
 * There is no client-driven acknowledgment the mock transport can block on for
 * server-initiated events (unlike command/response pairs), so
 * mock_dect_rpc_tr_expect_done() cannot be used here; it would block forever. Instead this
 * test:
 *
 *   1. Declares the exact expected event bytes via mock_dect_rpc_tr_expect_add(). Any
 *      MISMATCH is still caught synchronously inside the mock's send(), on whichever
 *      thread calls it (here: the system workqueue thread).
 *   2. Independently checks the real, synchronous l2_dect state (net_if_is_dormant())
 *      right after triggering -- this does not depend on timing and confirms the
 *      association codepath genuinely ran.
 *   3. Sleeps briefly to let the workqueue actually send the queued events before the
 *      test (and its teardown) moves on.
 *
 * Known gap: if the workqueue never ran at all, step 1 alone would not fail (the mock's
 * send() only asserts on a *mismatch*, not on "never called"); step 2 is what actually
 * guards against that for the l2_dect side.
 *
 * Associating/releasing also triggers *several* IF_ADDRS_CHANGED events per transition:
 * one from dect_rpc_server_link_state_changed() itself, plus one more per underlying
 * net_if_ipv6_addr_add/rm() or net_if_ipv6_prefix_add/rm() call l2_dect makes while
 * churning through IPv6 addressing (each independently observed by the separate
 * dect_rpc_server_ipv6_event_cb() net_mgmt listener). Concretely, on association,
 * dect_net_l2_ipv6_addressing_parent_added_handle() removes the old link-local address
 * and adds a new one (2 events), then dect_net_l2_ipv6_ula_sync_for_peer() adds a ULA
 * unicast address and its on-link prefix (2 more) -- and the reverse (address/prefix
 * *removal*) happens on release. So what is functionally "one" association/release
 * fans out to 4+ separate net_mgmt events, each independently producing an
 * IF_ADDRS_CHANGED notification. This is not wrong (the notification is a stateless
 * "please re-sync" with a constant payload, so redundant copies are harmless/idempotent
 * to the client) but it is noisy on the wire; debouncing multiple notifications queued
 * in quick succession into one could be a worthwhile follow-up in dect_rpc_if.c.
 * EXPECT_LINK_STATE_TRANSITION() below pads with enough identical expectations rather
 * than pin an exact count (see its own comment for the observed count and the mock's
 * cap in common/mock_dect_rpc_transport.c / CONFIG_MOCK_DECT_RPC_TR_MAX_EXPECTED_PKTS).
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
#include "rpc_test_link_state_expect.h"
#include "server_dect_fixture.h"

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/ztest.h>

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

ZTEST(dect_rpc_server_link_state, test_associate_sends_link_state_and_addrs_changed)
{
	/* carrier_ok=true after boot (mock HAL activate); dormant=false once associated. */
	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_FALSE);

	zassert_ok(server_dect_fixture_associate());

	/* Synchronous, timing-independent check: dect_net_l2_parent_association_created()
	 * calls net_if_dormant_off() directly (no workqueue involved).
	 */
	zassert_false(net_if_is_dormant(dect_iface), "iface should no longer be dormant");

	/* Let dect_rpc_evt_work (system workqueue) send the queued events. */
	k_msleep(100);
	zassert_true(mock_dect_rpc_tr_sent_count() > 0, "No link-state RPC event was sent");
	mock_dect_rpc_tr_expect_reset();

	/* Release so the next ZTEST case (which does its own associate) does not hit the
	 * "already associated" assert in dect_net_l2_parent_association_created() -- ztest
	 * case order is not relied upon elsewhere in this file, but cleaning up here keeps
	 * this test self-contained regardless.
	 */
	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_TRUE);
	zassert_ok(server_dect_fixture_release());
	k_msleep(100);
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_server_link_state, test_release_sends_link_state_and_addrs_changed)
{
	/* Associate first (self-contained: do not rely on ZTEST case ordering). Drop these
	 * events; only the *release* below is under test in this case.
	 */
	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_FALSE);
	zassert_ok(server_dect_fixture_associate());
	k_msleep(100);
	mock_dect_rpc_tr_expect_reset();

	zassert_false(net_if_is_dormant(dect_iface));

	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_TRUE);

	zassert_ok(server_dect_fixture_release());

	zassert_true(net_if_is_dormant(dect_iface), "iface should be dormant again after release");

	k_msleep(100);
	zassert_true(mock_dect_rpc_tr_sent_count() > 0, "No release link-state RPC event was sent");
	mock_dect_rpc_tr_expect_reset();
}

ZTEST_SUITE(dect_rpc_server_link_state, NULL, suite_setup, tc_setup, NULL, NULL);
