/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * App-domain RT framework - Zephyr application.
 *
 * Brings up:
 *  - the RT component (timer + zero-latency ISR toggling the RT GPIO),
 *  - optionally (CONFIG_BT) a BLE peripheral (LBS) that keeps MPSL/SDC and the
 *    radio active so the ZLI vs MPSL coexistence can be observed on SoCs where
 *    the radio shares the application core,
 *  - a shell command set ("rt ...") that exercises the control plane at
 *    runtime.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>

#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/lbs.h>
#endif

#include <dk_buttons_and_leds.h>

#include "rt_framework.h"

#define RUN_STATUS_LED         DK_LED1
#define CON_STATUS_LED         DK_LED2
#define RT_EVENT_LED           DK_LED3
#define RUN_LED_BLINK_INTERVAL 1000

#if defined(CONFIG_BT)
#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static struct k_work adv_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static void adv_work_handler(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	printk("Connected\n");
	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
	dk_set_led_off(CON_STATUS_LED);
}

static void recycled_cb(void)
{
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
	.recycled     = recycled_cb,
};

static struct bt_lbs_cb lbs_callbacks;
#endif /* CONFIG_BT */

#if defined(CONFIG_RT_FW_DATA_PLANE)
static bool rt_event_led_state;

static void rt_event_handler(uint32_t type, uint32_t value, uint32_t timestamp)
{
	ARG_UNUSED(type);
	ARG_UNUSED(value);
	ARG_UNUSED(timestamp);
	/* printk("RT event: type=%u value=%u ts=%u\n", type, value, timestamp); */

	rt_event_led_state = !rt_event_led_state;
	dk_set_led(RT_EVENT_LED, rt_event_led_state);
}
#endif

static int cmd_rt_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rt_fw_enable(true);
	shell_print(sh, "RT path enabled");
	return 0;
}

static int cmd_rt_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rt_fw_enable(false);
	shell_print(sh, "RT path disabled");
	return 0;
}

static int cmd_rt_freq(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long period_us;
	int parse_err = 0;
	int err;

	ARG_UNUSED(argc);

	period_us = shell_strtoul(argv[1], 0, &parse_err);
	if (parse_err != 0) {
		shell_error(sh, "Invalid period '%s'", argv[1]);
		return -EINVAL;
	}

	err = rt_fw_set_period_us((uint32_t)period_us);
	if (err) {
		shell_error(sh, "Failed to set period (err %d); allowed range %u..%u us",
			    err, RT_FW_MIN_PERIOD_US, RT_FW_MAX_PERIOD_US);
		return err;
	}

	shell_print(sh, "RT period requested: %lu us (effective within one period)",
		    period_us);
	return 0;
}

static int cmd_rt_status(const struct shell *sh, size_t argc, char **argv)
{
	struct rt_fw_status st;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = rt_fw_get_status(&st);
	if (err) {
		shell_error(sh, "Failed to read status (err %d)", err);
		return err;
	}

	shell_print(sh, "applied      : %s, %u us (%u ticks)",
		    st.enabled ? "enabled" : "disabled", st.period_us, st.period_ticks);
	shell_print(sh, "requested    : %s, %u us (%u ticks)",
		    st.req_enabled ? "enabled" : "disabled",
		    st.req_period_us, st.req_period_ticks);
	shell_print(sh, "pending      : %s", st.pending ? "yes" : "no");
	shell_print(sh, "command_seq  : %u", st.command_seq);
	shell_print(sh, "ack_seq      : %u", st.ack_seq);
	shell_print(sh, "tick_count   : %u", st.tick_count);
	shell_print(sh, "dropped_evts : %u", st.dropped_events);
	shell_print(sh, "last_error   : %u", st.last_error);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rt_cmds,
	SHELL_CMD(start, NULL, "Enable RT GPIO toggling", cmd_rt_start),
	SHELL_CMD(stop, NULL, "Disable RT GPIO toggling", cmd_rt_stop),
	SHELL_CMD_ARG(freq, NULL, "Set RT period in microseconds: rt freq <us>", cmd_rt_freq, 2, 0),
	SHELL_CMD(status, NULL, "Show RT status/telemetry", cmd_rt_status),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(rt, &rt_cmds, "App-domain RT framework control plane", NULL);

int main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting app-domain RT framework\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = rt_fw_init();
	if (err) {
		printk("RT framework init failed (err %d)\n", err);
		return 0;
	}

#if defined(CONFIG_RT_FW_DATA_PLANE)
	rt_fw_register_event_cb(rt_event_handler);
#endif

#if defined(CONFIG_BT)
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = bt_lbs_init(&lbs_callbacks);
	if (err) {
		printk("Failed to init LBS (err %d)\n", err);
		return 0;
	}

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();
#else
	printk("BLE coexistence load disabled (CONFIG_BT=n)\n");
#endif

	printk("RT framework ready. Use the 'rt' shell command to control it.\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
