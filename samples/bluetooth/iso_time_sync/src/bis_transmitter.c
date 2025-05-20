/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** BIS transmitter implementation for the time sync sample
 *
 * This file implements a device that sets up a BIS transmitter
 * with a configured number of streams.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>

#include "iso_time_sync.h"

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void bis_transmitter_start(uint8_t retransmission_number, uint16_t max_transport_latency_ms)
{
	int err;
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;

	iso_tx_init(retransmission_number, NULL);

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	/* Set the advertising data */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("failed to set advertising data (err %d)\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_FAST_INT_MIN_2,
							       BT_GAP_PER_ADV_FAST_INT_MAX_2,
							       BT_LE_PER_ADV_OPT_NONE));
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n",
		       err);
		return;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}

	struct bt_iso_big_create_param big_create_param = {
		.num_bis = CONFIG_BT_ISO_MAX_CHAN,
		.bis_channels = iso_tx_channels_get(),
		.interval = CONFIG_SDU_INTERVAL_US,
		.latency = max_transport_latency_ms,
		.packing = 0, /* 0 - sequential, 1 - interleaved */
		.framing = 0, /* 0 - unframed, 1 - framed */
	};

	/* Create BIG */
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		printk("Failed to create BIG (err %d)\n", err);
		return;
	}

	printk("BIS transmitter started\n");
}
