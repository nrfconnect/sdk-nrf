/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SR coexistence sample test bench
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"

#include <nrfx_clock.h>
#include "zephyr_fmac_main.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	int ret = 0;
	bool is_ant_mode_sep = IS_ENABLED(CONFIG_COEX_SEP_ANTENNAS);
	bool is_ble_central = IS_ENABLED(CONFIG_BT_ROLE_CENTRAL);
	bool is_wlan_server = IS_ENABLED(CONFIG_WIFI_ZPERF_SERVER);
	bool is_zperf_udp = IS_ENABLED(CONFIG_WIFI_ZPERF_PROT_UDP);
	bool test_wlan = IS_ENABLED(CONFIG_TEST_TYPE_WLAN);
	bool test_ble = IS_ENABLED(CONFIG_TEST_TYPE_BLE);

#if !defined(CONFIG_COEX_SEP_ANTENNAS) && \
	!(defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP))
	BUILD_ASSERT("Shared antenna support is not available with nRF7002 shields");
#endif

	wifi_memset_context();

	wifi_net_mgmt_callback_functions();

#if defined(CONFIG_BOARD_NRF7002DK_NRF7001_NRF5340_CPUAPP) || \
	defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP)
	#if defined(CONFIG_NRF700X_BT_COEX)
		/* Configure SR side (nRF5340 side) switch in nRF7002 DK */
		LOG_INF("Configure SR side (nRF5340 side) switch");
		ret = nrf_wifi_config_sr_switch(is_ant_mode_sep);
		if (ret != 0) {
			LOG_ERR("Unable to configure SR side switch: %d", ret);
			goto err;
		}
	#endif /* CONFIG_NRF700X_BT_COEX */
#endif

#if defined(CONFIG_NRF700X_BT_COEX)
	/* Configure non-PTA registers of Coexistence Hardware */
	LOG_INF("Configuring non-PTA registers.");
	ret = nrf_wifi_coex_config_non_pta(is_ant_mode_sep);
	if (ret != 0) {
		LOG_ERR("Configuring non-PTA registers of CoexHardware FAIL");
		goto err;
	}
#endif /* CONFIG_NRF700X_BT_COEX */

	if (IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_BLE_TP_CENTRAL) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_BLE_TP_CENTRAL) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_CLIENT_BLE_TP_PERIPH) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_CLIENT_BLE_TP_PERIPH) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_BLE_TP_CENTRAL) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_BLE_TP_CENTRAL) ||
	IS_ENABLED(CONFIG_WIFI_TP_UDP_SERVER_BLE_TP_PERIPH) ||
	IS_ENABLED(CONFIG_WIFI_TP_TCP_SERVER_BLE_TP_PERIPH)) {
		ret = wifi_tput_ble_tput(test_wlan, is_ant_mode_sep,
		test_ble, is_ble_central, is_wlan_server, is_zperf_udp);
	}
	if (ret != 0) {
		LOG_INF("Test case failed");
		goto err;
	}
	return 0;

err:
	LOG_INF("Returning with error");
	return ret;
}
