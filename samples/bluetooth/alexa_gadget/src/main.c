/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Alexa Gadgets sample
 */

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/services/gadgets_profile.h>
#include <bluetooth/gatt.h>
#include <dk_buttons_and_leds.h>
#include <settings/settings.h>

#ifndef CONFIG_BT_ALEXA_GADGETS_CAPABILITY_CUSTOM_NAMESPACE
#define CONFIG_BT_ALEXA_GADGETS_CAPABILITY_CUSTOM_NAMESPACE ""
#endif

#define BLE_CON_STATUS_LED DK_LED1
#define GADGET_CON_STATUS_LED DK_LED2
#define WAKEWORD_STATUS_LED DK_LED3
#define COLOR_CYCLE_LED DK_LED4

#define BOND_ERASE_BUTTON_NUM 4
#define BOND_ERASE_BUTTON_MSK CONCAT(DK_BTN, CONCAT(BOND_ERASE_BUTTON_NUM, _MSK))

static void led_toggle_func(struct k_timer *timer_id);

K_TIMER_DEFINE(led_toggle_timer, led_toggle_func, NULL);

static bool color_cycler_active;
static bool color_cycler_led_on;
static struct bt_conn *current_conn;

static void color_cycler_event_send(void);

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	if (err) {
		printk("MTU exchange error: %d\n", err);
	} else {
		bt_gadgets_profile_mtu_exchanged(conn);
	}
}

static struct bt_gatt_exchange_params exchange_params = {
	.func = exchange_func,
};

static void pairing_confirm(struct bt_conn *conn)
{
	int err;

	err = bt_conn_auth_pairing_confirm(conn);
	if (err) {
		printk("bt_conn_auth_pairing_confirm: %d\n", err);
	}
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.pairing_confirm = pairing_confirm,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);

	current_conn = bt_conn_ref(conn);

	dk_set_led_on(BLE_CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		dk_set_led_off(BLE_CON_STATUS_LED);
		dk_set_led_off(GADGET_CON_STATUS_LED);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	int mtu_err;

	mtu_err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (mtu_err) {
		printk("bt_gatt_exchange_mtu: %d\n", mtu_err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (color_cycler_active && (button_state & has_changed)) {
		/* Any key press used when color cycler is active */
		color_cycler_event_send();
		return;
	}
}

static void led_toggle_func(struct k_timer *timer_id)
{
	if (!color_cycler_active) {
		k_timer_stop(&led_toggle_timer);
		dk_set_led_off(COLOR_CYCLE_LED);
		return;
	}

	if (color_cycler_led_on) {
		dk_set_led_off(COLOR_CYCLE_LED);
		color_cycler_led_on = false;
	} else {
		dk_set_led_on(COLOR_CYCLE_LED);
		color_cycler_led_on = true;
	}
}

static void color_cycler_event_send(void)
{
	static const char * const color_names[] = {
		"Green", "Verdant green", "Lime green", "Celadon",
		"Chartreuse green", "Acid green", "Verdigris",
		"Myrtle green"
	};
	const char *color;
	char json_msg[64];
	int len;
	int err;

	/* The Color cycler skill expects an event message with the
	 * "ReportColor" name, and json payload format: {"color":"value"}
	 */

	/* Choose "random" color from list */
	color = color_names[k_uptime_get_32() % ARRAY_SIZE(color_names)];

	len = snprintf(json_msg, sizeof(json_msg), "{\"color\":\"%s\"}", color);
	if (len < 0 || len > sizeof(json_msg)) {
		__ASSERT_NO_MSG(false);
		printk("Format error\n");
		return;
	}

	err = bt_gadgets_profile_custom_event_json_send(
			"ReportColor",
			json_msg);
	if (err) {
		printk("Failed to send color event\n");
		return;
	}

	color_cycler_active = false;
}

static void color_cycler_directive_handler(
	const char *name, const char *payload, size_t payload_len)
{
	/* Handle "color cycler" messages from the custom skill described in
	 * this sample: https://github.com/alexa/Alexa-Gadgets-Raspberry-Pi-Samples/tree/v2.0.0/src/examples/color_cycler
	 */

	/* Typical message structure:
	 * {"colors_list":["RED","YELLOW","GREEN","CYAN","BLUE","PURPLE","WHITE"],"intervalMs":1000,"iterations":2,"startGame":true}
	 */

	/* The nRF5x-DKs only has green LEDs,
	 * so the actual colors in the message are ignored.
	 * One green LED will simply toggle at the requested intervalMs
	 * until the "StopLED" message is received,
	 * or the user presses any of the buttons.
	 */

	if (strcmp(
			CONFIG_BT_ALEXA_GADGETS_CAPABILITY_CUSTOM_NAMESPACE,
			"Custom.ColorCyclerGadget") != 0) {
		return;
	}

	if (strcmp(name, "StopLED") == 0) {
		color_cycler_active = false;
		return;
	}

	if (strstr(payload, "\"startGame\":true") == NULL) {
		return;
	}

	char *ptr;
	int interval;

	ptr = strstr(payload, "\"intervalMs\":");
	if (ptr == NULL) {
		return;
	}

	ptr += sizeof("\"intervalMs\":") - 1;
	if (ptr >= &payload[payload_len]) {
		return;
	}

	interval = strtol(ptr, NULL, 10);
	if (interval < 2) {
		return;
	}

	interval /= 2;
	color_cycler_active = true;
	color_cycler_led_on = true;

	dk_set_led_on(COLOR_CYCLE_LED);

	k_timer_start(
		&led_toggle_timer,
		K_MSEC(interval),
		K_MSEC(interval));
}

#if CONFIG_BT_ALEXA_GADGETS_CAPABILITY_STATELISTENER
static void stateupdate_directive_handler(const char *name, const char *val)
{
	if (strcmp(name, "wakeword") == 0 && strcmp(val, "active") == 0) {
		dk_set_led_on(WAKEWORD_STATUS_LED);
	} else if (strcmp(name, "wakeword") == 0 && strcmp(val, "cleared") == 0) {
		dk_set_led_off(WAKEWORD_STATUS_LED);
	}
}
#endif /* CONFIG_BT_ALEXA_GADGETS_CAPABILITY_STATELISTENER */

static void gadgets_handler(const struct bt_gadgets_evt *evt)
{
	__ASSERT_NO_MSG(evt != NULL);

	/* NOTE: This callback is executed directly from a Bluetooth handler.
	 * As such, stack size for this function depends on
	 * CONFIG_BT_RX_STACK_SIZE.
	 * Increase this stack size if needed.
	 */

	/* Directives are documented here:
	 * https://developer.amazon.com/en-US/docs/alexa/alexa-gadgets-toolkit/defined-interfaces.html
	 */

	switch (evt->type) {
	case BT_GADGETS_EVT_READY:
		printk("BT_GADGETS_EVT_READY\n");
		dk_set_led_on(GADGET_CON_STATUS_LED);
		break;
	case BT_GADGETS_EVT_SETALERT:
		printk("BT_GADGETS_EVT_SETALERT\n");
		break;
	case BT_GADGETS_EVT_DELETEALERT:
		printk("BT_GADGETS_EVT_DELETEALERT\n");
		break;
	case BT_GADGETS_EVT_SETINDICATOR:
		printk("BT_GADGETS_EVT_SETINDICATOR\n");
		break;
	case BT_GADGETS_EVT_CLEARINDICATOR:
		printk("BT_GADGETS_EVT_CLEARINDICATOR\n");
		break;
	case BT_GADGETS_EVT_STATEUPDATE:
		printk("BT_GADGETS_EVT_STATEUPDATE\n");
		/* "StateUpdate" directive */
#if CONFIG_BT_ALEXA_GADGETS_CAPABILITY_STATELISTENER
		/* Set/clear LED upon wakeword state changes */
		__ASSERT_NO_MSG(evt->parameters.state_update != NULL);

		for (int i = 0; i < evt->parameters.state_update->states_count; ++i) {
			stateupdate_directive_handler(
				evt->parameters.state_update->states[i].name,
				evt->parameters.state_update->states[i].value);
		}
#endif /* CONFIG_BT_ALEXA_GADGETS_CAPABILITY_STATELISTENER */
		break;
	case BT_GADGETS_EVT_SPEECHMARKS:
		printk("BT_GADGETS_EVT_SPEECHMARKS\n");
		break;
	case BT_GADGETS_EVT_MUSICTEMPO:
		printk("BT_GADGETS_EVT_MUSICTEMPO\n");
		break;
	case BT_GADGETS_EVT_CUSTOM:
		printk("BT_GADGETS_EVT_CUSTOM\n");
		if (IS_ENABLED(CONFIG_BT_ALEXA_GADGETS_CAPABILITY_CUSTOM)) {
			/* Custom directive */

			color_cycler_directive_handler(
				evt->parameters.custom_directive.name,
				evt->parameters.custom_directive.payload,
				evt->parameters.custom_directive.size);
		}
		break;

	case BT_GADGETS_EVT_CUSTOM_SENT:
		printk("BT_GADGETS_EVT_CUSTOM_SENT\n");
		break;
	default:
		printk("Unexpected event: %d\n", evt->type);
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	err = dk_leds_init();
	if (err) {
		printk("Cannot init LEDs (err: %d)\n", err);
	}
}

static void bond_count(const struct bt_bond_info *info, void *user_data)
{
	*((uint32_t *) user_data) += 1;
}

void main(void)
{
	int err;

	printk("Starting Alexa Gadgets example\n");

	configure_gpio();

	uint32_t buttons = dk_get_buttons();

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&conn_auth_callbacks);

	err = bt_enable(NULL);
	if (err) {
		printk("bt_enable: %d\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		err = settings_load();
		if (err) {
			printk("settings_load: %d\n", err);
		}
	}

	err = bt_gadgets_profile_init(gadgets_handler);
	if (err) {
		printk("gadgets_profile_init: %d\n", err);
		return;
	}

	if (buttons == BOND_ERASE_BUTTON_MSK) {
		printk("Erasing all bonds.\n");

		err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
		if (err) {
			printk("bt_unpair: %d\n", err);
		}
	}

	uint32_t peer_count = 0;

	bt_foreach_bond(BT_ID_DEFAULT, bond_count, &peer_count);

	if (peer_count > 0) {
		printk("Trying to reconnect to bonded peer\n");
		printk("Hold Button " STRINGIFY(BOND_ERASE_BUTTON_NUM)
		       " while resetting/power cycling to erase bond.\n");
	}

	err = bt_gadgets_profile_adv_start(peer_count > 0 ? false : true);
	if (err) {
		printk("gadgets_profile_adv_start: %d\n", err);
		return;
	}
}
