/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/ble_adv.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

/* This configuration file is included only once from ble_adv module and holds
 * information about advertising packets.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} ble_adv_def_include_once;

static const struct bt_data ad_unbonded[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	/* nRF Desktop devices are assumed to have transmit power set to 0 dBm
	 * during advertising.
	 */
	BT_DATA_BYTES(BT_DATA_TX_POWER, 0x0),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
			  (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
			  (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
#if CONFIG_DESKTOP_HIDS_ENABLE
			  0x12, 0x18,	/* HID Service */
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
			  0x0f, 0x18,	/* Battery Service */
#endif
	),

#if CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
			  0x06, 0x00,	/* Microsoft Vendor ID */
			  0x03,		/* Microsoft Beacon ID */
			  0x00,		/* Microsoft Beacon Sub Scenario */
			  0x80),	/* Reserved RSSI Byte */
#endif
};

static const struct bt_data ad_bonded[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
#if CONFIG_DESKTOP_HIDS_ENABLE
			  0x12, 0x18,	/* HID Service */
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
			  0x0f, 0x18,	/* Battery Service */
#endif
	),
};

/* When using BT_LE_ADV_OPT_USE_NAME, device name is added to scan response
 * data by controller.
 */
static const struct bt_data sd[] = {};

#if CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR
/* Ensure that grace period timeout is long enough for Swift Pair. */
BUILD_ASSERT(CONFIG_CAF_BLE_ADV_GRACE_PERIOD_TIMEOUT >= 30);
#endif

static int prepare_adv_data(struct caf_ble_adv_state *state, struct caf_ble_adv_data *data)
{
	if (state->bond_cnt > 0) {
		data->ad = ad_bonded;
		data->ad_size = ARRAY_SIZE(ad_bonded);
	} else {
		data->ad = ad_unbonded;
		data->ad_size = ARRAY_SIZE(ad_unbonded);
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_ADV_SWIFT_PAIR)) {
			if (state->grace_period) {
				/* Drop Swift Pair advertising data. */
				data->ad_size -= 1;
			} else {
				data->grace_period_needed = true;
			}
		}
	}

	data->sd = sd;
	data->sd_size = ARRAY_SIZE(sd);

	return 0;
}
