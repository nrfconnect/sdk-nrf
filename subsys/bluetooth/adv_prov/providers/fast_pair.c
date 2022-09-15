/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov.h>
#include <bluetooth/services/fast_pair.h>

#define ADV_DATA_BUF_SIZE	CONFIG_BT_ADV_PROV_FAST_PAIR_ADV_BUF_SIZE

static bool enabled = true;
static bool show_ui_pairing = IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_SHOW_UI_PAIRING);
static enum bt_fast_pair_adv_battery_mode adv_battery_mode = (
	IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_BATTERY_DATA_NONE) ?
		BT_FAST_PAIR_ADV_BATTERY_MODE_NONE :
			(IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_BATTERY_DATA_SHOW_UI) ?
				BT_FAST_PAIR_ADV_BATTERY_MODE_SHOW_UI_IND :
							BT_FAST_PAIR_ADV_BATTERY_MODE_HIDE_UI_IND)
);


void bt_le_adv_prov_fast_pair_enable(bool enable)
{
	enabled = enable;
}

void bt_le_adv_prov_fast_pair_show_ui_pairing(bool enable)
{
	show_ui_pairing = enable;
}

int bt_le_adv_prov_fast_pair_set_battery_mode(enum bt_fast_pair_adv_battery_mode mode)
{
	if ((mode < 0) || (mode >= BT_FAST_PAIR_ADV_BATTERY_MODE_COUNT)) {
		return -EINVAL;
	}

	adv_battery_mode = mode;

	return 0;
}

static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	static uint8_t buf[ADV_DATA_BUF_SIZE];
	struct bt_fast_pair_adv_config adv_config;

	if (!enabled) {
		return -ENOENT;
	}

	if (state->pairing_mode) {
		adv_config.adv_mode = BT_FAST_PAIR_ADV_MODE_DISCOVERABLE;
	} else {
		if (show_ui_pairing) {
			adv_config.adv_mode = BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_SHOW_UI_IND;
		} else {
			adv_config.adv_mode = BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_HIDE_UI_IND;
		}
	}

	adv_config.adv_battery_mode = adv_battery_mode;

	if (IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_AUTO_SET_PAIRING_MODE)) {
		/* Set Fast Pair pairing mode to match Fast Pair advertising mode. */
		bt_fast_pair_set_pairing_mode(adv_config.adv_mode ==
					      BT_FAST_PAIR_ADV_MODE_DISCOVERABLE);
	}

	__ASSERT_NO_MSG(bt_fast_pair_adv_data_size(adv_config) <= sizeof(buf));

	return bt_fast_pair_adv_data_fill(ad, buf, sizeof(buf), adv_config);
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(fast_pair, get_data);
