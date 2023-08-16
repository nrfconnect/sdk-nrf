/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/bluetooth/bluetooth.h>
#include "iso_broadcast_src.h"
#include "iso_broadcast_sink.h"
#include "nrfx_clock.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_ISO_TEST_LOG_LEVEL);

static int hfclock_config_and_start(void)
{
	int ret;

	/* Use this to turn on 128 MHz clock for cpu_app */
	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);

	ret -= NRFX_ERROR_BASE_NUM;
	if (ret) {
		return ret;
	}

	nrfx_clock_hfclk_start();
	while (!nrfx_clock_hfclk_is_running()) {
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
