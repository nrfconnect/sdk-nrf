/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <dk_buttons_and_leds.h>

#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/lbs.h>
#endif

#include "timer_internal.h"

#define RUN_STATUS_LED         DK_LED1
#define CONNECTION_STATUS_LED  DK_LED2
#define EVENT_STATUS_LED       DK_LED3
#define RUN_LED_INTERVAL_MS    1000

#if defined(CONFIG_BT)
#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static struct k_work advertising_work;

static const struct bt_data advertising_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data scan_data[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static void advertising_handler(struct k_work *work)
{
	int error;

	ARG_UNUSED(work);
	error = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, advertising_data,
				ARRAY_SIZE(advertising_data), scan_data,
				ARRAY_SIZE(scan_data));
	printk("Advertising start: %d\n", error);
}

static void connected(struct bt_conn *connection, uint8_t error)
{
	ARG_UNUSED(connection);
	if (error == 0U) {
		dk_set_led_on(CONNECTION_STATUS_LED);
	}
}

static void disconnected(struct bt_conn *connection, uint8_t reason)
{
	ARG_UNUSED(connection);
	ARG_UNUSED(reason);
	dk_set_led_off(CONNECTION_STATUS_LED);
}

static void recycled(void)
{
	k_work_submit(&advertising_work);
}

BT_CONN_CB_DEFINE(connection_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

static struct bt_lbs_cb lbs_callbacks;
#endif

static bool event_led_state;

static void event_handler(const struct rtfw_event *event, void *user_data)
{
	ARG_UNUSED(user_data);

	if (event->type == RTFW_EVENT_COMMAND_PROCESSED ||
	    event->type == RTFW_TIMER_EVENT_TICK) {
		event_led_state = !event_led_state;
		dk_set_led(EVENT_STATUS_LED, event_led_state);
	}
}

static int command_start(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int error = rtfw_timer_enable(true);

	shell_print(shell, "RTFW timer start requested (%d)", error);
	return error;
}

static int command_stop(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int error = rtfw_timer_enable(false);

	shell_print(shell, "RTFW timer stop requested (%d)", error);
	return error;
}

static int command_period(const struct shell *shell, size_t argc, char **argv)
{
	int parse_error = 0;
	unsigned long period_us;
	int error;

	ARG_UNUSED(argc);
	period_us = shell_strtoul(argv[1], 0, &parse_error);
	if (parse_error != 0 ||
	    period_us > CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US) {
		shell_error(shell, "Invalid period '%s'", argv[1]);
		return -EINVAL;
	}

	error = rtfw_timer_period_set((uint32_t)period_us);
	if (error != 0) {
		shell_error(shell, "Allowed range: %u..%u us",
			    CONFIG_SAMPLE_RTFW_TIMER_MIN_PERIOD_US,
			    CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US);
		return error;
	}

	shell_print(shell, "RTFW timer period requested: %lu us", period_us);
	return 0;
}

static int command_status(const struct shell *shell, size_t argc, char **argv)
{
	struct rtfw_timer_status status;
	int error;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	error = rtfw_timer_status_get(&status);
	if (error != 0) {
		return error;
	}

	shell_print(shell, "applied   : %s, %u us (%u ticks)",
		    status.enabled ? "running" : "stopped",
		    status.period_us, status.period_ticks);
	shell_print(shell, "requested : %s, %u us (%u ticks)",
		    status.requested_enabled ? "running" : "stopped",
		    status.requested_period_us, status.requested_period_ticks);
	shell_print(shell, "pending=%s ticks=%u dropped=%u max_depth=%u faults=0x%x",
		    status.pending ? "yes" : "no", status.tick_count,
		    status.dropped_events, status.max_queue_depth, status.faults);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rtfw_commands,
	SHELL_CMD(start, NULL, "Start the TIMER/GPIO backend", command_start),
	SHELL_CMD(stop, NULL, "Stop the TIMER/GPIO backend", command_stop),
	SHELL_CMD_ARG(period, NULL, "Set period in microseconds", command_period, 2, 0),
	SHELL_CMD(status, NULL, "Show coherent backend status", command_status),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(rtfw, &rtfw_commands, "RTFW timer/GPIO control", NULL);

int main(void)
{
	int error;
	bool led = false;

	printk("Starting event-driven RTFW timer/GPIO backend\n");
	error = dk_leds_init();
	if (error != 0) {
		return error;
	}

	error = rtfw_timer_init(event_handler, NULL);
	if (error != 0) {
		printk("RTFW initialization failed: %d\n", error);
		return error;
	}
#if defined(CONFIG_BT)
	error = bt_enable(NULL);
	if (error != 0) {
		return error;
	}
	error = bt_lbs_init(&lbs_callbacks);
	if (error != 0) {
		return error;
	}
	k_work_init(&advertising_work, advertising_handler);
	k_work_submit(&advertising_work);
#endif

	for (;;) {
		led = !led;
		dk_set_led(RUN_STATUS_LED, led);
		k_sleep(K_MSEC(RUN_LED_INTERVAL_MS));
	}
}
