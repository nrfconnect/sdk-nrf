/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/bluetooth/bluetooth.h>
#include "iso_broadcast_src.h"
#include "iso_broadcast_sink.h"
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_ISO_TEST_LOG_LEVEL);

static const uint32_t hfclk_requested_frequency = MHZ(128);

static int hfclock_config_and_start(void)
{
	/* Use this to turn on 128 MHz clock for cpu_app */
	clock_control_set_rate(DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk), NULL,
			       &hfclk_requested_frequency);

	nrf_clock_hfclk_t clk_src;

	nrfx_clock_hfclk_start();
	while (!nrfx_clock_hfclk_running_check(&clk_src)) {
	}

	return 0;
}


int main(void)
{
	int err;

	LOG_INF("Starting ISO tester");

	hfclock_config_and_start();

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	err = iso_broadcast_src_init();
	if (err) {
		LOG_ERR("iso_broadcast_src_init failed (err %d)", err);
	}

	err = iso_broadcast_sink_init();
	if (err) {
		LOG_ERR("iso_broadcaster_sink_init failed (err %d)", err);
	}

	return 0;
}
