/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/services/nus.h>
#include <caf/ble_adv.h>

/* This configuration file is included only once from ble_adv module and holds
 * information about advertising packets.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} ble_adv_def_include_once;

#define ADV_POS_BT_FLAGS	(0)

static const struct bt_data flags_unbonded =
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR));
static const struct bt_data flags_bonded = BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR);

static struct bt_data ad[] = {
	/* Placeholder for advertising flags. */
	{}
};

/* When using BT_LE_ADV_OPT_USE_NAME, device name is added to scan response
 * data by controller.
 */
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static int prepare_adv_data(struct caf_ble_adv_state *state, struct caf_ble_adv_data *data)
{
	if (state->bond_cnt > 0) {
		memcpy(&ad[ADV_POS_BT_FLAGS], &flags_bonded, sizeof(flags_bonded));
	} else {
		memcpy(&ad[ADV_POS_BT_FLAGS], &flags_unbonded, sizeof(flags_unbonded));
	}

	data->ad = ad;
	data->ad_size = ARRAY_SIZE(ad);

	data->sd = sd;
	data->sd_size = ARRAY_SIZE(sd);

	return 0;
}
