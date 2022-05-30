/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/ble_adv.h>
#include <bluetooth/services/fast_pair.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

/* This configuration file is included only once from ble_adv module and holds
 * information about advertising packets.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} ble_adv_def_include_once;

enum adv_pos {
	ADV_POS_BT_FLAGS,
	ADV_POS_TX_POWER,
	ADV_POS_GAP_APPEARANCE,
	ADV_POS_SWIFT_PAIR = IS_ENABLED(CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR) ?
			     (ADV_POS_GAP_APPEARANCE + 1) : (ADV_POS_GAP_APPEARANCE),
	ADV_POS_FAST_PAIR = IS_ENABLED(CONFIG_DESKTOP_BLE_ADV_FAST_PAIR) ?
			    (ADV_POS_SWIFT_PAIR + 1) : (ADV_POS_SWIFT_PAIR),
};

#define ADV_SIZE_CORE	(ADV_POS_GAP_APPEARANCE + 1)

static const struct bt_data flags_unbonded =
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR));
static const struct bt_data flags_bonded = BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR);

static struct bt_data ad[] = {
	/* Placeholder for advertising flags. */
	{},
	/* nRF Desktop devices are assumed to have transmit power set to 0 dBm
	 * during advertising.
	 */
	BT_DATA_BYTES(BT_DATA_TX_POWER, 0x0),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
		      (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
#if CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
		      0x06, 0x00,   /* Microsoft Vendor ID */
		      0x03,         /* Microsoft Beacon ID */
		      0x00,         /* Microsoft Beacon Sub Scenario */
		      0x80),        /* Reserved RSSI Byte */
#endif
#if CONFIG_DESKTOP_BLE_ADV_FAST_PAIR
        {},
#endif
};

/* When using BT_LE_ADV_OPT_USE_NAME, device name is added to scan response
 * data by controller.
 */
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
#if CONFIG_DESKTOP_HIDS_ENABLE
			  0x12, 0x18,	/* HID Service */
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
			  0x0f, 0x18,	/* Battery Service */
#endif
	),
};

#if CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR
/* Ensure that grace period timeout is long enough for Swift Pair. */
BUILD_ASSERT(CONFIG_CAF_BLE_ADV_GRACE_PERIOD_TIMEOUT >= 30);
#endif


static int fast_pair_adv_prepare(struct bt_data *adv_data, bool fp_discoverable)
{
	/* Make sure that Fast Pair data was freed and set to NULL to prevent memory leaks. */
	if (adv_data->data) {
		k_free((void *)adv_data->data);
		adv_data->data = NULL;
	}

	/* Fast Pair pairing mode must be set manually. */
	bt_fast_pair_set_pairing_mode(fp_discoverable);

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

static int prepare_adv_data(struct caf_ble_adv_state *state, struct caf_ble_adv_data *data)
{
	int err = 0;

	data->ad = ad;
	data->ad_size = ADV_SIZE_CORE;

	if (state->bond_cnt > 0) {
		memcpy(&ad[ADV_POS_BT_FLAGS], &flags_bonded, sizeof(flags_bonded));
	} else {
		memcpy(&ad[ADV_POS_BT_FLAGS], &flags_unbonded, sizeof(flags_unbonded));
	}

	if (!state->grace_period && (state->bond_cnt == 0)) {
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR)) {
			__ASSERT_NO_MSG(data->ad_size == ADV_POS_SWIFT_PAIR);

			/* Append Swift Pair advertising data. */
			data->ad_size += 1;
			data->grace_period_needed = true;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_ADV_FAST_PAIR)) {
			__ASSERT_NO_MSG(data->ad_size == ADV_POS_FAST_PAIR);

			bool fp_discoverable = (bt_fast_pair_get_account_key_count() == 0);

			err = fast_pair_adv_prepare(&ad[ADV_POS_FAST_PAIR], fp_discoverable);
			data->ad_size += 1;
			data->grace_period_needed = true;
		}
	}

	data->sd = sd;
	data->sd_size = ARRAY_SIZE(sd);

	return err;
}
