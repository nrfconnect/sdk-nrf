/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** This file implements receiving SDUs for the time sync demo
 *
 * When a valid SDU is received two things are done:
 *  1. A LED is toggled immediately
 *  2. A timer trigger is configured to toggle another LED
 *     CONFIG_TIMED_LED_PRESENTATION_DELAY_US after the received timestamp.
 *     This will ensure that all receivers toggle it at the same time.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include "iso_time_sync.h"

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf);
static void iso_connected(struct bt_iso_chan *chan);
static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason);

static void (*iso_chan_disconnected_cb)(void);

static struct gpio_dt_spec led_sdu_received = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

static struct bt_iso_chan_ops iso_ops = {
	.recv = iso_recv,
	.connected = iso_connected,
	.disconnected = iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_rx_qos = {
	.sdu = SDU_SIZE_BYTES,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos iso_qos = {
	.rx = &iso_rx_qos,
};

static struct bt_iso_chan iso_chan[] = {
	{
		.ops = &iso_ops,
		.qos = &iso_qos,
	},
};

static struct bt_iso_chan *iso_channels[] = {
	&iso_chan[0],
};

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	if ((info->flags & BT_ISO_FLAGS_VALID) == 0) {
		/* The packet should be valid. */
		return;
	}

	if ((info->flags & BT_ISO_FLAGS_TS) == 0) {
		/* The packet should contain a timestamp */
		return;
	}

	if (buf->len != SDU_SIZE_BYTES) {
		return;
	}

	uint32_t counter = net_buf_remove_le32(buf);
	uint8_t btn_pressed = net_buf_remove_u8(buf);

	if (IS_ENABLED(CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE)) {
		gpio_pin_set_dt(&led_sdu_received, btn_pressed);
	}

	uint32_t trigger_time_us = info->ts + CONFIG_TIMED_LED_PRESENTATION_DELAY_US;

	timed_led_toggle_trigger_at(btn_pressed, trigger_time_us);

	if (counter % 100 == 0) {
		uint32_t current_time_us = controller_time_us_get() & UINT32_MAX;
		int32_t time_to_trigger = trigger_time_us - current_time_us;

		printk("Recv SDU counter %u with timestamp %u us, controller_time %u us, btn_val: %d, LED will be set in %d us\n",
		       counter, info->ts, current_time_us, btn_pressed, time_to_trigger);
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};

	int err;
	struct bt_conn_info conn_info;
	struct bt_iso_info iso_info;

	err = bt_iso_chan_get_info(chan, &iso_info);
	if (err) {
		printk("Failed obtaining ISO info\n");
		return;
	}

	err = bt_conn_get_info(chan->iso, &conn_info);
	if (err) {
		printk("Failed obtaining conn info\n");
		return;
	}

	printk("ISO Channel connected: ");
	iso_chan_info_print(&iso_info, conn_info.role);

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
	if (err != 0) {
		printk("Failed to setup ISO RX data path: %d\n", err);
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel disconnected with reason 0x%02x\n", reason);

	if (iso_chan_disconnected_cb) {
		iso_chan_disconnected_cb();
	}
}

void iso_rx_init(uint8_t retransmission_number, void (*iso_disconnected_cb)(void))
{
	int err;

	iso_chan_disconnected_cb = iso_disconnected_cb;

	iso_rx_qos.rtn = retransmission_number;

	if (IS_ENABLED(CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE)) {
		err = gpio_pin_configure_dt(&led_sdu_received, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n", err,
				led_sdu_received.port->name, led_sdu_received.pin);
		}
	}
}

struct bt_iso_chan **iso_rx_channels_get(void)
{
	return iso_channels;
}
