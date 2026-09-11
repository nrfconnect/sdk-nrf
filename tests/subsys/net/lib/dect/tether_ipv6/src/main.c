/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/byteorder.h>

#include "tether_host_sim.h"
#include "tether_net_fixture.h"
#include "tether_test_eth.h"

#include "dect_tether_ipv6_int.h"

static struct tether_net_fixture fx;
static struct tether_host_ctx host;
static bool tether_skip_next_tx_reset = true;

static void *tether_suite_setup(void)
{
	int err = tether_net_fixture_setup(&fx);

	zassert_equal(err, 0, "network fixture setup failed: %d", err);
	tether_host_ctx_init(&host);
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
	if (!dect_tether_ipv6_dhcpv6_srv_lease_bound()) {
		zassert_ok(tether_host_send_solicit_to(fx.eth, &host, &fx.server_ll));
		(void)tether_host_wait_tx_growth(tether_tx_capture_count, 5000);
		zassert_ok(tether_host_send_request(fx.eth, &host, &fx.server_ll));
		(void)tether_host_wait_tx_growth(tether_tx_capture_count, 5000);
		zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound());
	}

	tether_test_eth_tx_reset();
	zassert_ok(tether_net_fixture_parent_release(&fx), "parent release failed");

	baseline = tether_tx_capture_count;
	zassert_ok(tether_host_send_rs(fx.eth, &host), "RS inject after uplink loss");
	(void)tether_host_wait_tx_growth(baseline, 3000);

	zassert_true(tether_host_find_ra(baseline, &ra), "RA after uplink loss not seen");
	zassert_equal(ra.router_lifetime, 0, "router lifetime should be 0 on uplink loss");
	zassert_false(ra.managed, "Managed flag should be cleared on uplink loss");
}

ZTEST(dect_tether_ipv6_integration, test_renew_address_change_revoke)
{
	struct tether_dhcp_parse dhcp;
	int baseline;
	int tx_end;

	if (!dect_tether_ipv6_dhcpv6_srv_lease_bound()) {
		zassert_ok(tether_host_send_solicit_to(fx.eth, &host, &fx.server_ll));
		(void)tether_host_wait_tx_growth(tether_tx_capture_count, 5000);
		zassert_ok(tether_host_send_request(fx.eth, &host, &fx.server_ll));
		(void)tether_host_wait_tx_growth(tether_tx_capture_count, 5000);
		zassert_true(dect_tether_ipv6_dhcpv6_srv_lease_bound());
	}

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
}
