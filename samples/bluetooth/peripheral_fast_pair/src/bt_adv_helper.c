/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/services/fast_pair.h>
#include "bt_tx_power_adv.h"
#include "bt_adv_helper.h"

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define TX_POWER_ADV_DATA_POS	(ARRAY_SIZE(ad) - 2)
#define FAST_PAIR_ADV_DATA_POS	(ARRAY_SIZE(ad) - 1)

/* The Bluetooth Core Specification v5.2 (Vol. 4, Part E, 7.8.45) allows for time range between
 * 1 second and 3600 seconds. In case of Fast Pair we should avoid RPA address rotation when device
 * is Fast Pair discoverable. If not discoverable, the RPA address rotation should be done together
 * with Fast Pair payload update (responsibility of sample/application).
 */
BUILD_ASSERT(CONFIG_BT_RPA_TIMEOUT == 3600);

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* Empty placeholder for TX power advertising data. */
	{
	},
	/* Empty placeholder for Fast Pair advertising data. */
	{
	},
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static int bt_adv_helper_fast_pair_prepare(struct bt_data *adv_data, bool fp_discoverable)
{
	/* Make sure that Fast Pair data was freed and set to NULL to prevent memory leaks. */
	if (adv_data->data) {
		k_free((void *)adv_data->data);
		adv_data->data = NULL;
	}

	size_t buf_size = bt_fast_pair_adv_data_size(fp_discoverable);
	uint8_t *buf = k_malloc(buf_size);

	if (!buf) {
		return -ENOMEM;
	}

	int err = bt_fast_pair_adv_data_fill(adv_data, buf, buf_size, fp_discoverable);

	if (err) {
		k_free(buf);
	}

	return err;
}

static int bt_adv_helper_tx_power_prepare(struct bt_data *adv_data)
{
	/* Make sure that TX power data was freed and set to NULL to prevent memory leaks. */
	if (adv_data->data) {
		k_free((void *)adv_data->data);
		adv_data->data = NULL;
	}

	size_t buf_size = bt_tx_power_adv_data_size();
	uint8_t *buf = k_malloc(buf_size);

	if (!buf) {
		return -ENOMEM;
	}

	int err = bt_tx_power_adv_data_fill(adv_data, buf, buf_size);

	if (err) {
		k_free(buf);
	}

	return err;
}

int bt_adv_helper_adv_start(bool fp_discoverable)
{
	int err = bt_adv_helper_adv_stop();

	if (err) {
		printk("Cannot stop advertising (err: %d)\n", err);
		return err;
	}

	err = bt_adv_helper_fast_pair_prepare(&ad[FAST_PAIR_ADV_DATA_POS], fp_discoverable);
	if (err) {
		printk("Cannot prepare Fast Pair advertising data (err: %d)\n", err);
		return err;
	}

	err = bt_adv_helper_tx_power_prepare(&ad[TX_POWER_ADV_DATA_POS]);
	if (err) {
		printk("Cannot prepare TX power advertising data (err: %d)\n", err);
		return err;
	}

	/* According to Fast Pair specification, the advertising interval should be no longer than
	 * 100 ms when discoverable and no longer than 250 ms when not discoverable.
	 */
	static const struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME),
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
		.peer = NULL,
	};

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	return err;
}

int bt_adv_helper_adv_stop(void)
{
	return bt_le_adv_stop();
}
