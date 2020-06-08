/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <dk_buttons_and_leds.h>
#include <logging/log.h>

#include "coap_client_utils.h"

LOG_MODULE_REGISTER(coap_client, CONFIG_COAP_CLIENT_LOG_LEVEL);

#define OT_CONNECTION_LED DK_LED1
#define MTD_SED_LED DK_LED3

static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_on(OT_CONNECTION_LED);
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_off(OT_CONNECTION_LED);
}

static void on_mtd_mode_toggle(u32_t val)
{
	dk_set_led(MTD_SED_LED, val);
}

static void on_button_changed(u32_t button_state, u32_t has_changed)
{
	u32_t buttons = button_state & has_changed;

	if (buttons & DK_BTN1_MSK) {
		coap_client_toggle_one_light();
	}

	if (buttons & DK_BTN2_MSK) {
		coap_client_toggle_mesh_lights();
	}

	if (buttons & DK_BTN3_MSK) {
		coap_client_toggle_minimal_sleepy_end_device();
	}

	if (buttons & DK_BTN4_MSK) {
		coap_client_send_provisioning_request();
	}
}

void main(void)
{
	int ret;

	LOG_INF("Start CoAP-client sample");

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
		return;
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Connot init leds, (error: %d)", ret);
		return;
	}

	coap_client_utils_init(on_ot_connect, on_ot_disconnect,
			       on_mtd_mode_toggle);
}
