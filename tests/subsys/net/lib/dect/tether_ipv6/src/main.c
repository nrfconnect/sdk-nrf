/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/byteorder.h>

#include "tether_host_sim.h"
#include "tether_net_fixture.h"
#include "tether_test_eth.h"

#include "dect_tether_ipv6_int.h"

static struct tether_net_fixture fx;
static struct tether_host_ctx host;
static struct tether_host_ctx host_other;
static bool tether_skip_next_tx_reset = true;

/* Bootstraps a bound lease for `host` if one isn't already active, reusing
 * the Solicit/Request exchange verified by test_dhcp_solicit_request_bind.
 */
static void tether_ensure_lease_bound(void)
{
	struct tether_dhcp_parse dhcp;
	int baseline;

	if (dect_tether_ipv6_dhcpv6_srv_lease_bound()) {
		return;
	}

	/* Ztest runs test cases in alphabetical, not declaration, order, so this
	 * bootstrap cannot assume test_dhcp_solicit_request_bind already ran and
	 * populated host.server_duid: capture it from the Advertise here too.
	 */
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_solicit_to(fx.eth, &host, &fx.server_ll));
	(void)tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_ADVERTISE, &dhcp, &host),
		     "Advertise not captured while bootstrapping a lease");

	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_request(fx.eth, &host, &fx.server_ll));
	(void)tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound());
}

/* Waits until the server-side lease becomes unbound, polling like the
 * dhcp_thread() itself does. Used to deterministically reach the
 * "no active binding" code paths regardless of prior test ordering.
 */
static bool tether_wait_lease_unbound(int timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < end) {
		if (!dect_tether_ipv6_dhcpv6_srv_lease_bound()) {
			return true;
		}
		k_msleep(20);
	}

	return false;
}

static void *tether_suite_setup(void)
{
	int err = tether_net_fixture_setup(&fx);

	zassert_equal(err, 0, "network fixture setup failed: %d", err);
	tether_host_ctx_init(&host);
	tether_host_ctx_init_other(&host_other, 0x66);
	return NULL;
}

static void tether_suite_teardown(void *unused)
{
	ARG_UNUSED(unused);
	tether_net_fixture_teardown(&fx);
}

static void tether_before(void *unused)
{
	ARG_UNUSED(unused);

	if (!tether_skip_next_tx_reset) {
		tether_test_eth_tx_reset();
	}
	tether_skip_next_tx_reset = false;
}

ZTEST_SUITE(dect_tether_ipv6_integration, NULL, tether_suite_setup, tether_before, NULL,
	    tether_suite_teardown);

ZTEST(dect_tether_ipv6_integration, test_bootstrap_init)
{
	struct tether_ra_parse ra;

	zassert_true(fx.tether_started, "tether should be started in suite setup");
	zassert_not_null(fx.dect, "dect iface");
	zassert_not_null(fx.eth, "eth iface");
	zassert_true(tether_host_find_ra(0, &ra), "init-time RA should be captured during setup");
	zassert_equal(ra.router_lifetime, 0, "router lifetime 0 before DECT parent");
	zassert_false(ra.managed, "Managed flag clear before DECT parent");
}

ZTEST(dect_tether_ipv6_integration, test_rs_to_ra)
{
	struct tether_ra_parse ra;
	int baseline = tether_tx_capture_count;
	int tx_end;

	zassert_ok(tether_host_send_rs(fx.eth, &host), "RS inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 3000);
	zassert_true(tx_end > baseline, "expected RA TX after RS");

	zassert_true(tether_host_find_ra(baseline, &ra), "RA not found in TX capture");
	zassert_true(ra.router_lifetime > 0, "router lifetime should be non-zero");
	zassert_true(ra.managed, "Managed (M) flag should be set");
#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
	zassert_true(ra.has_rdnss, "RDNSS option expected");
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
	zassert_true(ra.has_pio, "PIO expected when uplink + DECT addresses present");
#endif
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_solicit_request_bind)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	/* Optional RS (matches typical host ordering). */
	zassert_ok(tether_host_send_rs(fx.eth, &host));
	(void)tether_host_wait_tx_growth(tether_tx_capture_count, 2000);
	tether_test_eth_tx_reset();

	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_solicit_to(fx.eth, &host, &fx.server_ll),
		   "Solicit inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Advertise after Solicit");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_ADVERTISE, &dhcp, &host),
		     "Advertise not captured");
	zassert_true(dhcp.has_iaaddr && (dhcp.have_ula || dhcp.have_gua),
		     "Advertise should carry IAADDR");

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_request(fx.eth, &host, &fx.server_ll), "Request inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Request");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Reply not captured");
	zassert_true(dhcp.has_iaaddr && (dhcp.have_ula || dhcp.have_gua),
		     "Reply should carry IAADDR");

	zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound(), "lease should be bound");
}

ZTEST(dect_tether_ipv6_integration, test_uplink_loss_ra_withdraw)
{
	struct tether_ra_parse ra;
	int baseline;

	/* Ensure lease exists for realistic disconnect scenario. */
	tether_ensure_lease_bound();

	tether_test_eth_tx_reset();
	zassert_ok(tether_net_fixture_parent_release(&fx), "parent release failed");

	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_rs(fx.eth, &host), "RS inject after uplink loss");
	(void)tether_host_wait_tx_growth(baseline, 3000);

	zassert_true(tether_host_find_ra(baseline, &ra), "RA after uplink loss not seen");
	zassert_equal(ra.router_lifetime, 0, "router lifetime should be 0 on uplink loss");
	zassert_false(ra.managed, "Managed flag should be cleared on uplink loss");

	/* Restore the parent association: this suite's default (non-shuffled) test
	 * order happens to run this test last, but nothing guarantees that stays
	 * true, so leave the fixture as it was found rather than relying on it.
	 */
	zassert_ok(tether_net_fixture_parent_restore(&fx), "parent restore failed");
}

ZTEST(dect_tether_ipv6_integration, test_renew_rebind_address_change_revoke)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	tether_ensure_lease_bound();

	zassert_ok(tether_net_fixture_change_gua_prefix(&fx), "GUA prefix change failed");
	k_msleep(100);

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_renew(fx.eth, &host, &fx.server_ll), "Renew inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Renew");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Renew Reply not captured");
	zassert_true(tether_host_dhcp_has_revoked_iaaddr(baseline, DHCPV6_MSG_REPLY),
		     "Renew Reply should include revoked IAADDR (lifetime 0) after GUA change");

	/* Same check for dhcp_reply_revoke_plan()'s Rebind branch: change the
	 * prefix again (back to the original) so the lease cached from the
	 * Renew above is guaranteed stale, regardless of what other tests in
	 * this suite (which run in alphabetical, not declaration, order) may
	 * have done to the prefix.
	 */
	zassert_ok(tether_net_fixture_revert_gua_prefix(&fx), "GUA prefix revert failed");
	k_msleep(100);

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_rebind(fx.eth, &host, &fx.server_ll), "Rebind inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Rebind");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Rebind Reply not captured");
	zassert_true(tether_host_dhcp_has_revoked_iaaddr(baseline, DHCPV6_MSG_REPLY),
		     "Rebind Reply should include revoked IAADDR (lifetime 0) after GUA change");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_rebind_after_bind)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	tether_ensure_lease_bound();

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_rebind(fx.eth, &host, &fx.server_ll), "Rebind inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Rebind");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Rebind Reply not captured");
	zassert_true(dhcp.has_iaaddr && (dhcp.have_ula || dhcp.have_gua),
		     "Rebind Reply should carry IAADDR");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_confirm_after_bind)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	tether_ensure_lease_bound();

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_confirm(fx.eth, &host, &fx.server_ll), "Confirm inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Confirm");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Confirm Reply not captured");
	zassert_false(dhcp.has_iaaddr,
		     "Confirm Reply should carry Status=Success only, no IA_NA/IAADDR");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_info_request_no_addrs)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	/* Common case: Info-Request with a Client ID. */
	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_info_request(fx.eth, &host, &fx.server_ll, true),
		   "Info-Request inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Info-Request");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Info-Request Reply not captured");
	zassert_false(dhcp.has_iaaddr, "Info-Request Reply should not carry IA_NA/IAADDR");

	/* RFC 8415 §18.2.6: Client ID is only RECOMMENDED for Info-Request, and
	 * it is the only message type the server accepts without one.
	 */
	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_info_request(fx.eth, &host, &fx.server_ll, false),
		   "Info-Request (no Client ID) inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Reply after Info-Request without Client ID");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_REPLY, &dhcp, NULL),
		     "Info-Request (no Client ID) Reply not captured");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_rebind_confirm_without_lease_silent)
{
	int baseline;

	tether_ensure_lease_bound();

	/* Force the lease into the unbound state deterministically, mirroring
	 * test_uplink_loss_ra_withdraw, instead of relying on suite ordering.
	 */
	zassert_ok(tether_net_fixture_parent_release(&fx), "parent release failed");
	zassert_true(tether_wait_lease_unbound(3000),
		     "lease should become unbound after uplink loss");

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_rebind(fx.eth, &host, &fx.server_ll), "Rebind inject failed");
	zassert_equal(tether_host_wait_tx_growth(baseline, 1000), -ETIMEDOUT,
		     "Rebind without a bound lease must be silently dropped");

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_confirm(fx.eth, &host, &fx.server_ll), "Confirm inject failed");
	zassert_equal(tether_host_wait_tx_growth(baseline, 1000), -ETIMEDOUT,
		     "Confirm without a bound lease must be silently dropped");

	zassert_ok(tether_net_fixture_parent_restore(&fx), "parent restore failed");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_other_client_solicit_no_addrs_when_bound)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	tether_ensure_lease_bound();

	/* NoAddrsAvail replies skip the production neighbor-install step (only
	 * granted addresses get one), so the fake Ethernet driver's lack of a
	 * real ND responder would otherwise strand the reply. Pre-seed it.
	 */
	zassert_ok(tether_host_install_neighbor(fx.eth, &host_other),
		   "failed to pre-seed neighbor cache for other client");

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_solicit_to(fx.eth, &host_other, &fx.server_ll),
		   "Other-client Solicit inject failed");
	tx_end = tether_host_wait_tx_growth(baseline, 5000);
	zassert_true(tx_end > baseline, "expected Advertise for other client");
	zassert_true(tether_host_find_dhcp(baseline, DHCPV6_MSG_ADVERTISE, &dhcp, &host_other),
		     "Advertise for other client not captured");
	zassert_false(dhcp.has_iaaddr,
		     "Other client should get NoAddrsAvail (no IAADDR) while a lease is bound");

	zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound(),
		     "existing lease should remain bound after other-client Solicit");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_other_client_request_silent_when_bound)
{
	int baseline;

	tether_ensure_lease_bound();

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_request(fx.eth, &host_other, &fx.server_ll),
		   "Other-client Request inject failed");
	zassert_equal(tether_host_wait_tx_growth(baseline, 1000), -ETIMEDOUT,
		     "Request from a different client while another lease is bound must be "
		     "dropped");

	zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound(),
		     "existing lease should remain bound after other-client Request");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_missing_clientid_silent)
{
	int baseline;

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_solicit_no_clientid(fx.eth, &host, &fx.server_ll),
		   "Solicit (no Client ID) inject failed");
	zassert_equal(tether_host_wait_tx_growth(baseline, 1000), -ETIMEDOUT,
		     "a non-Info-Request message without a Client ID must be silently dropped");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_serverid_mismatch_silent)
{
	int baseline;

	tether_ensure_lease_bound();

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_request_bad_serverid(fx.eth, &host, &fx.server_ll),
		   "Request (bad Server ID) inject failed");
	zassert_equal(tether_host_wait_tx_growth(baseline, 1000), -ETIMEDOUT,
		     "a Request/Renew with a mismatching Server ID must be silently dropped");

	zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound(),
		     "existing lease should remain bound after a Server ID mismatch");
}

ZTEST(dect_tether_ipv6_integration, test_dhcp_unknown_msgtype_silent)
{
	int baseline;

	tether_test_eth_tx_reset();
	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_unknown_type(fx.eth, &host, &fx.server_ll),
		   "unknown-type message inject failed");
	zassert_equal(tether_host_wait_tx_growth(baseline, 1000), -ETIMEDOUT,
		     "an unhandled DHCPv6 message type must be silently ignored");
}
