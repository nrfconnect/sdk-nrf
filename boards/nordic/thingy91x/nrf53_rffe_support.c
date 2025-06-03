/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <mpsl/mpsl_clock_ctrl_cb.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec rffe_spec =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), rffe_ble_wifi_enable_gpios);

static void hfclk_requested_cb(void)
{
	gpio_pin_set_dt(&rffe_spec, 1);
}

static void hfclk_released_cb(void)
{
	gpio_pin_set_dt(&rffe_spec, 0);
}

/**
 * Initialize the rffe_ble_wifi_enable GPIO pin and register callbacks for HFCLK events.
 *
 * This will cause the BLE/Wi-Fi switch (U22) to be powered whenever MPSL is requesting the
 * high-frequency clock (HFCLK). For any wireless use supported by MPSL (e.g. BLE),
 * this ensures that the RF path is ready before radio operations begin.
 */
static int rffe_init(void)
{
	int ret;

	if (!device_is_ready(rffe_spec.port)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&rffe_spec, (GPIO_OUTPUT_INACTIVE));
	if (ret < 0) {
		return ret;
	}

	mpsl_clock_ctrl_cb_register_hfclk_requested(hfclk_requested_cb);
	mpsl_clock_ctrl_cb_register_hfclk_released(hfclk_released_cb);
	return 0;
}

SYS_INIT(rffe_init, POST_KERNEL, CONFIG_MPSL_INIT_PRIORITY);
