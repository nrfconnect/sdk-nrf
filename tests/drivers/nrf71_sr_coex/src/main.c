/* main.c - build-only exercise of the nRF71 SR coexistence driver API */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <common/fw_if/nrf71_coex_if.h>
#include <common/fw_if/nrf71_cd_sr_if.h>

/*
 * This test builds the standalone nRF71 SR coexistence driver together with the
 * nRF71 Wi-Fi driver and exercises the coex_cd_* API contract so the driver
 * links against the Wi-Fi FMAC coexistence transport and the Short-Range driver
 * stubs.
 */
int main(void)
{
	struct coex_sr_sw_client_params_t sr_req = {
		.sw_client_request = SR_SW_CLIENT_REQUEST,
		.sw_client_pti_level = SR_SW_CLIENT_REQ_PTI_HIGH,
		.sw_client_type = SR_CONNECTION,
		.request_timeout_in_ms = 50,
		.sr_operating_band = SR_BAND_2PT4G,
	};
	enum coex_sr_sw_client_req_status_t grant_status;
	struct short_range_activity_info_t activity = {
		.sr_activity_type = SR_BLE_SCAN,
		.sr_activity_action = SR_ACTIVITY_START,
		.start_time_of_activity = 0,
		.activity_interval = 30,
		.activity_duration = 30,
		.activity_timeout = 10000,
	};

	(void)coex_cd_sr_software_client_request(&sr_req, &grant_status);
	(void)coex_cd_update_short_range_activity_info(&activity);
	(void)coex_cd_sr_power_notify(COEX_SR_POWERED_UP_READY);
	(void)coex_cd_sr_power_notify(COEX_SR_PREPARE_POWER_DOWN);
	(void)coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY);
	(void)coex_cd_wifi_power_notify(COEX_WIFI_PREPARE_POWER_DOWN);

	printk("nRF71 SR coexistence driver built\n");

	return 0;
}
