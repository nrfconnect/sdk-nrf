/**
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "main.h"

int setup_interfaces(void)
{
	const struct device *dev = device_get_binding("wlan0");
	/* Without this, DHCPv4 starts on first interface and if that is not Wi-Fi or
	 * only supports IPv6, then its an issue. (E.g., OpenThread)
	 *
	 * So, DHCPv4 is started on Wi-Fi interface always, independent of the ordering.
	 */
	struct net_if *wifi_iface = net_if_lookup_by_dev(dev);

	/* As both are Ethernet, need to set specific interface */
	net_if_set_default(wifi_iface);

	net_config_init_app(dev, "Initializing network");

	return 0;
}

int main(void)
{
	int ret = 0;
	bool is_ant_mode_sep = IS_ENABLED(CONFIG_COEX_SEP_ANTENNAS);
	bool is_ot_client = IS_ENABLED(CONFIG_OT_ROLE_CLIENT);
	bool is_wifi_server = IS_ENABLED(CONFIG_WIFI_ZPERF_SERVER);
	bool is_wifi_zperf_udp = IS_ENABLED(CONFIG_WIFI_ZPERF_PROT_UDP);
	bool is_ot_zperf_udp = IS_ENABLED(CONFIG_OT_ZPERF_PROT_UDP);
	bool test_wifi = IS_ENABLED(CONFIG_TEST_TYPE_WLAN);
	bool test_thread = IS_ENABLED(CONFIG_TEST_TYPE_OT);
	bool is_sr_protocol_ble = IS_ENABLED(CONFIG_SR_PROTOCOL_BLE);

	/* LOG_INF("Set up interfaces"); */
	ret = setup_interfaces();
	if (ret) {
		LOG_INF("Failed to setup ifaces: %d\n", ret);
		return -1;
	}

#if !defined(CONFIG_COEX_SEP_ANTENNAS) && !(CONFIG_NRF70_SR_COEX_RF_SWITCH)
	BUILD_ASSERT("Shared antenna support is not available with nRF7002 shields");
#endif

	/* register callback functions */
	wifi_init();

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
	/* Configure SR side (nRF5340 side) switch in nRF700x */
	/* LOG_INF("Configure SR side switch"); */
	ret = nrf_wifi_config_sr_switch(is_ant_mode_sep);
	if (ret != 0) {
		LOG_ERR("Unable to configure SR side switch: %d", ret);
		return ret;
	}
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */

#if defined(CONFIG_NRF70_SR_COEX)
	/* Configure non-PTA registers of Coexistence Hardware */
	/* LOG_INF("Configuring non-PTA registers."); */
	ret = nrf_wifi_coex_config_non_pta(is_ant_mode_sep, is_sr_protocol_ble);
	if (ret != 0) {
		LOG_ERR("Configuring non-PTA registers of CoexHardware FAIL");
		return ret;
	}
#endif /* CONFIG_NRF70_SR_COEX */

	ret = wifi_tput_ot_tput(test_wifi, is_ant_mode_sep,
		test_thread, is_ot_client, is_wifi_server, is_wifi_zperf_udp, is_ot_zperf_udp,
		is_sr_protocol_ble);

	if (ret != 0) {
		LOG_INF("Test case failed. Returning with error");
		return ret;
	}

	/* Disable coexistence hardware */
	nrf_wifi_coex_hw_reset();

	LOG_INF("\nCoexistence test complete\n");

	return 0;
}
