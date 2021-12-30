/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <ram_pwrdn.h>
#include <device.h>
#include <pm/device.h>

#include "coap_client_utils.h"

#if CONFIG_BT_NUS
#include "ble_utils.h"
#endif

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#include <dfu/mcuboot.h>
#endif

LOG_MODULE_REGISTER(coap_client, CONFIG_COAP_CLIENT_LOG_LEVEL);

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

#define OT_CONNECTION_LED DK_LED1
#define BLE_CONNECTION_LED DK_LED2
#define MTD_SED_LED DK_LED3

#if CONFIG_BT_NUS

#define COMMAND_REQUEST_UNICAST 'u'
#define COMMAND_REQUEST_MULTICAST 'm'
#define COMMAND_REQUEST_PROVISIONING 'p'

static void on_nus_received(struct bt_conn *conn, const uint8_t *const data,
			    uint16_t len)
{
	if (len != 1) {
		LOG_WRN("Received invalid data length (%hd) from NUS", len);
		return;
	}

	LOG_INF("Received data: %c", data[0]);

	switch (*data) {
	case COMMAND_REQUEST_UNICAST:
		coap_client_toggle_one_light();
		break;

	case COMMAND_REQUEST_MULTICAST:
		coap_client_toggle_mesh_lights();
		break;

	case COMMAND_REQUEST_PROVISIONING:
		coap_client_send_provisioning_request();
		break;

	default:
		LOG_WRN("Received invalid data from NUS");
	}
}

static void on_ble_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_on(BLE_CONNECTION_LED);
}

static void on_ble_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_off(BLE_CONNECTION_LED);
}

#endif /* CONFIG_BT_NUS */

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

static void on_mtd_mode_toggle(uint32_t med)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	const struct device *cons = device_get_binding(CONSOLE_LABEL);

	if (med) {
		pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
	} else {
		pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	}
#endif
	dk_set_led(MTD_SED_LED, med);
}

static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

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

	if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
		power_down_unused_ram();
	}

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	/* Check if the image is run in the REVERT mode and eventually
	 * confirm it to prevent reverting on the next boot.
	 */
	if (mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT) {
		if (boot_write_img_confirmed()) {
			LOG_ERR("Confirming firmware image failed, it will be reverted on the next "
				"boot.");
		} else {
			LOG_INF("New device firmware image confirmed.");
		}
	}
#endif

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
		return;
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Cannot init leds, (error: %d)", ret);
		return;
	}

#if CONFIG_BT_NUS
	struct bt_nus_cb nus_clbs = {
		.received = on_nus_received,
		.sent = NULL,
	};

	ret = ble_utils_init(&nus_clbs, on_ble_connect, on_ble_disconnect);
	if (ret) {
		LOG_ERR("Cannot init BLE utilities");
		return;
	}

#endif /* CONFIG_BT_NUS */

	coap_client_utils_init(on_ot_connect, on_ot_disconnect,
			       on_mtd_mode_toggle);
}
