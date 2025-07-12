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

static void clock_event_cb(mpsl_clock_ctrl_event_t event)
{
	if (event == MPSL_CLOCK_CTRL_EVENT_HFCLK_REQUESTED) {
		gpio_pin_set_dt(&rffe_spec, 1);
	} else if (event == MPSL_CLOCK_CTRL_EVENT_HFCLK_RELEASED) {
		gpio_pin_set_dt(&rffe_spec, 0);
	}
}

/**
 * Initialize the rffe_ble_wifi_enable GPIO pin and register callback for clock events.
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

	mpsl_clock_ctrl_cb_register(clock_event_cb);
	return 0;
}

SYS_INIT(rffe_init, POST_KERNEL, CONFIG_MPSL_INIT_PRIORITY);
