/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>

#include <nrf_modem_dect.h>

extern void dect_mdm_ctrl_mdm_on_modem_lib_init(int ret, void *ctx);

#include "mock_nrf_modem_dect_mac.h"
#include "test_dect_utils.h"
#include "tether_dect_events.h"
#include "tether_net_fixture.h"
#include "tether_test_eth.h"

#include <dect_tether_ipv6.h>
#include "dect_tether_ipv6_int.h"

LOG_MODULE_REGISTER(test_dect_tether, CONFIG_TEST_DECT_TETHER_LOG_LEVEL);

#define TETHER_TEST_BEACON_CHANNEL      1657
#define TETHER_TEST_BEACON_SHORT_RD_ID  0x1234
#define TETHER_TEST_BEACON_LONG_RD_ID   0x56789ABC
#define TETHER_TEST_BEACON_NETWORK_ID   0x9876

static const uint8_t tether_gua_prefix_a[8] = { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0x01 };
static const uint8_t tether_gua_prefix_b[8] = { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0x02 };

static struct net_if *tether_get_dect_iface(void)
{
	int idx = net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME);

	if (idx < 0) {
		return NULL;
	}

	return net_if_get_by_index(idx);
}

static int tether_assign_gua_prefix(struct net_if *dect, const uint8_t prefix[8])
{
	struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params ntf = { 0 };
	struct net_in6_addr ula, gua;
	bool have_ula, have_gua;

	memcpy(ntf.ipv6_config.address, prefix, 8);
	ntf.ipv6_config.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX;

	if (mock_ntf_callbacks.ipv6_config_update_ntf == NULL) {
		return -ENODEV;
	}

	mock_ntf_callbacks.ipv6_config_update_ntf(&ntf);
	k_msleep(200);

	if (!dect_tether_ipv6_dect_addrs_get(dect, &ula, &gua, &have_ula, &have_gua) || !have_gua) {
		LOG_ERR("GUA not available from dect after prefix update");
		return -ENOENT;
	}

	return 0;
}

int tether_net_fixture_setup(struct tether_net_fixture *fx)
{
	struct test_dect_scan_result scan = { 0 };
	struct test_dect_scan_beacon_params beacon = {
		.channel = TETHER_TEST_BEACON_CHANNEL,
		.transmitter_short_rd_id = TETHER_TEST_BEACON_SHORT_RD_ID,
		.transmitter_long_rd_id = TETHER_TEST_BEACON_LONG_RD_ID,
		.network_id = TETHER_TEST_BEACON_NETWORK_ID,
		.mcs = 1,
		.transmit_power = 8,
		.rssi_2 = -45,
		.snr = 20,
	};
	struct dect_scan_params scan_params = {
		.channel_count = 1,
		.channel_list = { TETHER_TEST_BEACON_CHANNEL },
		.channel_scan_time_ms = 100,
	};
	struct test_dect_association_result assoc = { 0 };
	struct in6_addr eth_ll;
	int err;

	memset(fx, 0, sizeof(*fx));

	err = tether_dect_events_init();
	if (err != 0) {
		return err;
	}

	fx->dect = tether_get_dect_iface();
	fx->eth = tether_test_eth_iface();
	if (fx->dect == NULL || fx->eth == NULL) {
		LOG_ERR("iface missing dect=%p eth=%p", fx->dect, fx->eth);
		return -ENODEV;
	}

	/* Same entry point as integration tests: simulate NRF_MODEM_LIB_ON_INIT. */
	dect_mdm_ctrl_mdm_on_modem_lib_init(0, NULL);
	if (k_sem_take(&tether_activation_done_sem, K_MSEC(500)) != 0 &&
	    !dect_activate_done_received) {
		LOG_ERR("DECT stack did not activate");
		return -ETIMEDOUT;
	}

#if defined(CONFIG_NET_INTERFACE_NAME)
	(void)net_if_set_name(fx->eth, CONFIG_DECT_TETHER_IPV6_HOST_ETH_IFACE);
#endif

	err = net_if_up(fx->eth);
	if (err != 0 && err != -EALREADY) {
		return err;
	}

	err = tether_test_eth_wait_ll(fx->eth, &eth_ll, 3000);
	if (err != 0) {
		return err;
	}
	fx->eth_ll = eth_ll;

	err = dect_tether_ipv6_init();
	if (err != 0 && err != -EALREADY) {
		return err;
	}
	fx->server_ll = fx->eth_ll;
	fx->tether_started = true;

	/* DHCP thread waits for host eth link-local. */
	k_msleep(300);

	{
		struct dect_settings reset = { 0 };

		reset.cmd_params.reset_to_driver_defaults = true;
		if (net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, fx->dect, &reset,
			     sizeof(reset)) != 0) {
			return -EIO;
		}
	}

	tether_dect_events_reset_flags();
	err = test_dect_network_scan(fx->dect, &scan_params, &beacon, true, &scan);
	if (err != 0 || !scan.beacon_data_valid) {
		return err != 0 ? err : -EIO;
	}

	tether_dect_events_reset_flags();
	err = test_dect_association_request(fx->dect, TETHER_TEST_BEACON_LONG_RD_ID, NULL, true,
					    true, &assoc);
	if (err != 0 || !assoc.association_created_received) {
		return err != 0 ? err : -EIO;
	}

	err = tether_assign_gua_prefix(fx->dect, tether_gua_prefix_a);
	if (err != 0) {
		return err;
	}

	return 0;
}

void tether_net_fixture_teardown(struct tether_net_fixture *fx)
{
	if (fx->tether_started) {
		dect_tether_ipv6_deinit();
		fx->tether_started = false;
	}
	tether_dect_events_deinit();
}

int tether_net_fixture_parent_release(struct tether_net_fixture *fx)
{
	struct test_dect_association_release_result rel = { 0 };
	int err;

	tether_dect_events_reset_flags();
	err = test_dect_association_release(fx->dect, TETHER_TEST_BEACON_LONG_RD_ID, true, true,
					    &rel);
	if (err != 0) {
		return err;
	}

	k_msleep(200);
	return rel.association_changed_received ? 0 : -EIO;
}

int tether_net_fixture_change_gua_prefix(struct tether_net_fixture *fx)
{
	return tether_assign_gua_prefix(fx->dect, tether_gua_prefix_b);
}
