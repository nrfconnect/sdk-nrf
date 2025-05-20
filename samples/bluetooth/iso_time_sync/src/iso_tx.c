/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** This file implements transmitting SDUs for the time sync demo
 *
 * Each SDU contains a counter and the current value of a button.
 * The implementation ensures the following:
 *
 * 1. The SDU is transmitted right before it is supposed to be
 *    sent on air.
 * 2. The SDU is transmitted on all the connected isochronous channels
 *    with the same timestamp. This ensures that all the receivers
 *    can toggle their corresponding LEDs at the same time.
 * 3. A LED is toggled when the data is sent. This can be used
 *    to observe the end-to-end latency.s
 * 4. A LED is toggled when the LED is supposed to be toggled on the
 *    receiving side.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/iso.h>
#include <bluetooth/hci_vs_sdc.h>
#include <sdc_hci.h>

#include "iso_time_sync.h"

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static struct gpio_dt_spec led_on_sdu_send = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

static void iso_sent(struct bt_iso_chan *chan);
static void iso_connected(struct bt_iso_chan *chan);
static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason);

static void (*iso_chan_connected_cb)(void);

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_MAX_CHAN,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static bool first_sdu_sent;
static uint32_t tx_sdu_timestamp_us;
static uint32_t num_sdus_sent;

static void sdu_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sdu_work, sdu_work_handler);

static struct bt_iso_chan_ops iso_ops = {
	.connected = iso_connected,
	.sent = iso_sent,
	.disconnected = iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = SDU_SIZE_BYTES,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx_qos,
};

/* Create lists of ISO channels and pointers to ISO channels. */
#define _ISO_CHANNEL(_n, _)  {.ops = &iso_ops, .qos = &iso_qos}
static struct bt_iso_chan iso_channels[] = {
	LISTIFY(CONFIG_BT_ISO_MAX_CHAN, _ISO_CHANNEL, (,))
};

#define _ISO_CHANNEL_PTR(_n, _) &iso_channels[_n]
static struct bt_iso_chan *iso_channel_pointers[] = {
	LISTIFY(CONFIG_BT_ISO_MAX_CHAN, _ISO_CHANNEL_PTR, (,))
};

static bool iso_channels_awaiting_iso_sent_cb[CONFIG_BT_ISO_MAX_CHAN];

/* Store info about the ISO channels in use. */
static uint8_t roles[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_info iso_infos[CONFIG_BT_ISO_MAX_CHAN];

void iso_chan_info_print(struct bt_iso_info *info, uint8_t role)
{
	if (info->type == BT_ISO_CHAN_TYPE_CENTRAL || info->type == BT_ISO_CHAN_TYPE_PERIPHERAL) {
		uint8_t bn;
		uint32_t flush_timeout;
		uint32_t transport_latency_us;

		if (role == BT_CONN_ROLE_CENTRAL) {
			bn = info->can_send ?
				info->unicast.central.bn :
				info->unicast.peripheral.bn;
			flush_timeout = info->can_send ?
				info->unicast.central.flush_timeout :
				info->unicast.peripheral.flush_timeout;
			transport_latency_us = info->can_send ?
				info->unicast.central.latency :
				info->unicast.peripheral.latency;
		} else {
			bn = info->can_send ?
				info->unicast.peripheral.bn :
				info->unicast.central.bn;
			flush_timeout = info->can_send ?
				info->unicast.peripheral.flush_timeout :
				info->unicast.central.flush_timeout;
			transport_latency_us = info->can_send ?
				info->unicast.peripheral.latency :
				info->unicast.central.latency;
		}

		printk("interval: %d ms, NSE: %d, BN: %d, FT: %d, transport latency: %d us\n",
				info->iso_interval * 1250 / 1000,
				info->max_subevent,
				bn,
				flush_timeout / info->iso_interval,
				transport_latency_us);
	} else if (info->type == BT_ISO_CHAN_TYPE_BROADCASTER) {
		printk("interval: %d ms, NSE: %d, BN: %d, IRC: %d, PTO: %d, transport latency: %d us\n",
				info->iso_interval * 1250 / 1000,
				info->max_subevent,
				info->broadcaster.bn,
				info->broadcaster.irc,
				info->broadcaster.pto / info->iso_interval,
				info->broadcaster.latency);
	} else if (info->type == BT_ISO_CHAN_TYPE_SYNC_RECEIVER) {
		printk("interval: %d ms, NSE: %d, BN: %d, IRC: %d, PTO: %d, transport latency: %d us\n",
				info->iso_interval * 1250 / 1000,
				info->max_subevent,
				info->sync_receiver.bn,
				info->sync_receiver.irc,
				info->sync_receiver.pto / info->iso_interval,
				info->sync_receiver.latency);
	} else {
		printk("Unknown ISO channel type %d\n", info->type);
	}
}

static int iso_tx_time_stamp_get(struct bt_conn *conn, uint32_t *time_stamp)
{
	int err;
	uint16_t conn_handle = 0;
	sdc_hci_cmd_vs_iso_read_tx_timestamp_t params;
	sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t rsp;

	err = bt_hci_get_conn_handle(conn, &conn_handle);
	if (err) {
		printk("Failed to get conn_handle %d\n", err);
		return err;
	}

	params.conn_handle = conn_handle;

	err = hci_vs_sdc_iso_read_tx_timestamp(&params, &rsp);
	if (err) {
		return err;
	}

	*time_stamp = rsp.tx_time_stamp;

	return 0;
}

static int send_next_sdu(struct bt_iso_chan *chan, bool btn_pressed)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (!buf) {
		printk("Data buffer allocate timeout\n");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_u8(buf, btn_pressed);
	net_buf_add_le32(buf, num_sdus_sent);

	if (first_sdu_sent) {
		return bt_iso_chan_send_ts(chan, buf, 0, tx_sdu_timestamp_us);
	} else {
		return bt_iso_chan_send(chan, buf, 0);
	}

	return 0;
}

static void send_next_sdu_on_all_channels(void)
{
	int err;
	bool btn_pressed = (bool)gpio_pin_get_dt(&button);

	if (IS_ENABLED(CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE)) {
		gpio_pin_set_dt(&led_on_sdu_send, btn_pressed);
	}

	for (size_t i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {

		if (iso_channels[i].state != BT_ISO_STATE_CONNECTED) {
			continue;
		}

		err = send_next_sdu(&iso_channels[i], btn_pressed);
		if (err) {
			printk("Failed sending SDU, counter %d, index %d\n", num_sdus_sent, i);
			return;
		}

		/* Toggle that we are awaiting iso_sent callback on this channel */
		iso_channels_awaiting_iso_sent_cb[i] = true;

		if (!first_sdu_sent) {
			/* The first time we send an SDU, we send it without timestamp.
			 * After that we always send SDUs with timestamps to ensure they
			 * are time synchronized on all channels.
			 *
			 * To avoid reading the timestamp both after sending SDUs and after
			 * the first SDU, we break out early here the very first event.
			 */
			first_sdu_sent = true;
			break;
		}
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
	uint8_t chan_index = ARRAY_INDEX(iso_channels, chan);

	err = bt_iso_chan_get_info(chan, &iso_infos[chan_index]);
	if (err) {
		printk("Failed obtaining ISO info\n");
		return;
	}

	err = bt_conn_get_info(chan->iso, &conn_info);
	if (err) {
		printk("Failed obtaining conn info\n");
		return;
	}

	roles[chan_index] = conn_info.role;

	printk("ISO channel index %d connected: ", chan_index);
	iso_chan_info_print(&iso_infos[chan_index], roles[chan_index]);

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR, &hci_path);
	if (err != 0) {
		printk("Failed to setup ISO TX data path: %d\n", err);
	}

	if (iso_chan_connected_cb) {
		iso_chan_connected_cb();
	}

	if (!first_sdu_sent) {
		/* Send the first SDU immediately.
		 * The following SDUs will be sent time-aligned with the controller clock.
		 */
		send_next_sdu_on_all_channels();
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	uint32_t chan_index = ARRAY_INDEX(iso_channels, chan);

	printk("ISO Channel index %d disconnected with reason 0x%02x\n",
	       chan_index, reason);

	iso_channels_awaiting_iso_sent_cb[chan_index] = false;
}

static bool send_more_sdus_with_same_timestamp(struct bt_iso_chan *chan)
{
	if (first_sdu_sent) {
		struct bt_iso_chan *last_chan = NULL;

		/** Check if this is the last ISO channel where we are awaiting
		 * an iso_sent callback. If that is the case, that callback will
		 * be used to send SDUs on all ISO channels.
		 */
		for (size_t i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
			if (iso_channels_awaiting_iso_sent_cb[i]) {
				last_chan = &iso_channels[i];
			}
		}

		if (chan != last_chan) {
			return true;
		} else {
			return false;
		}
	}

	return false;
}

static uint32_t trigger_time_us_get(uint32_t sdu_sync_ref, uint8_t chan_index)
{
	uint32_t trigger_time_us = sdu_sync_ref + CONFIG_TIMED_LED_PRESENTATION_DELAY_US;

	/* The ISO type determines when to trigger the LED
	 * because of how the SDU synchronization reference is defined.
	 * See Bluetooth Core Specification, Vol 6, Part G, Section 3.2.
	 */

	if (iso_infos[chan_index].type == BT_ISO_CHAN_TYPE_CENTRAL ||
	    iso_infos[chan_index].type == BT_ISO_CHAN_TYPE_PERIPHERAL) {
		if (roles[chan_index] == BT_CONN_ROLE_CENTRAL) {
			trigger_time_us += iso_infos[chan_index].unicast.central.latency;
		}
	} else {
		trigger_time_us += iso_infos[chan_index].broadcaster.latency;
	}

	return trigger_time_us;
}

static void iso_sent(struct bt_iso_chan *chan)
{
	int err;
	uint32_t assigned_timestamp;

	if (send_more_sdus_with_same_timestamp(chan)) {
		return;
	}

	err = iso_tx_time_stamp_get(chan->iso, &assigned_timestamp);
	if (err) {
		printk("Failed obtaining timestamp\n");
		return;
	}

	uint8_t chan_index = ARRAY_INDEX(iso_channels, chan);

	/* Set the LED to toggle in sync with all the receivers. */
	bool btn_pressed = (bool)gpio_pin_get_dt(&button);
	uint32_t trigger_time_us = trigger_time_us_get(assigned_timestamp,
						       chan_index);
	timed_led_toggle_trigger_at(btn_pressed, trigger_time_us);

	/* Set up a timer to trigger right before the next set of SDUs are to be sent. */
	uint32_t controller_time_us = controller_time_us_get() & UINT32_MAX;
	int32_t time_to_next_sdu_us =
		assigned_timestamp + CONFIG_SDU_INTERVAL_US
		- controller_time_us
		- HCI_ISO_TX_SDU_ARRIVAL_MARGIN_US;

	/** Update the sent SDU counter before starting
	 * the timer that triggers when the next SDU is sent.
	 * This is done to ensure that counter is updated in time.
	 */
	uint32_t prev_sent_sdu = num_sdus_sent;

	num_sdus_sent++;

	iso_channels_awaiting_iso_sent_cb[chan_index] = false;

	k_work_schedule(&sdu_work, K_USEC(time_to_next_sdu_us));

	/* Increment the SDU timestamp with one SDU interval. */
	tx_sdu_timestamp_us = assigned_timestamp + CONFIG_SDU_INTERVAL_US;

	if (prev_sent_sdu % 100 == 0) {
		int32_t time_to_trigger = trigger_time_us - controller_time_us;

		printk("Sent SDU counter %u with timestamp %u us, controller_time %u us, ",
			   prev_sent_sdu, assigned_timestamp, controller_time_us);
		printk("btn_val: %d LED will be set in %d us\n",
			   btn_pressed, time_to_trigger);
	}
}

static void sdu_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	send_next_sdu_on_all_channels();
}

void iso_tx_init(uint8_t retransmission_number, void (*iso_connected_cb)(void))
{
	int err;
	iso_chan_connected_cb = iso_connected_cb;

	iso_tx_qos.rtn = retransmission_number;

	if (IS_ENABLED(CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE)) {
		err = gpio_pin_configure_dt(&led_on_sdu_send, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n", err,
				led_on_sdu_send.port->name, led_on_sdu_send.pin);
		}
	}

	err = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (err != 0) {
		printk("Error %d: failed to configure %s pin %d\n", err, button.port->name,
		       button.pin);
	}
}

struct bt_iso_chan **iso_tx_channels_get(void)
{
	return iso_channel_pointers;
}
