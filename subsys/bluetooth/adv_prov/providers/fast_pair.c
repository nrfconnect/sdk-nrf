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


void bt_le_adv_prov_fast_pair_enable(bool enable)
{
	enabled = enable;
}

void bt_le_adv_prov_fast_pair_show_ui_pairing(bool enable)
{
	show_ui_pairing = enable;
}

static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	static uint8_t buf[ADV_DATA_BUF_SIZE];
	enum bt_fast_pair_adv_mode adv_mode;

	if (!enabled) {
		return -ENOENT;
	}

	if (state->pairing_mode) {
		adv_mode = BT_FAST_PAIR_ADV_MODE_DISCOVERABLE;
	} else {
		if (show_ui_pairing) {
			adv_mode = BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_SHOW_UI_IND;
		} else {
			adv_mode = BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_HIDE_UI_IND;
		}
	}

	if (IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_AUTO_SET_PAIRING_MODE)) {
		/* Set Fast Pair pairing mode to match Fast Pair advertising mode. */
		bt_fast_pair_set_pairing_mode(adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE);
	}

	__ASSERT_NO_MSG(bt_fast_pair_adv_data_size(adv_mode) <= sizeof(buf));

	return bt_fast_pair_adv_data_fill(ad, buf, sizeof(buf), adv_mode);
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(fast_pair, get_data);
