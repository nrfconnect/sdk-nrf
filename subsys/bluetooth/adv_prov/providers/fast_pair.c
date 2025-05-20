/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov.h>
#include <bluetooth/adv_prov/fast_pair.h>
#include <bluetooth/services/fast_pair/fast_pair.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_le_adv_prov, CONFIG_BT_ADV_PROV_LOG_LEVEL);

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

#if CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING
void bt_le_adv_prov_fast_pair_show_ui_pairing(bool enable)
{
	show_ui_pairing = enable;
}
#endif /* CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING */

int bt_le_adv_prov_fast_pair_set_battery_mode(enum bt_fast_pair_adv_battery_mode mode)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_BN)) {
		LOG_ERR("Cannot set battery mode without support for the Battery Notification "
			"extension. Enable the CONFIG_BT_FAST_PAIR_BN Kconfig to use this API");

		return -ENOSYS;
	}

	if ((mode < 0) || (mode >= BT_FAST_PAIR_ADV_BATTERY_MODE_COUNT)) {
		return -EINVAL;
	}

	adv_battery_mode = mode;

	return 0;
}

static bool detect_discoverable_stop_condition(enum bt_fast_pair_adv_mode adv_mode,
					       const struct bt_le_adv_prov_adv_state *state)
{
	static bool drop_disc_adv_payload;
	static enum bt_fast_pair_adv_mode last_adv_mode = BT_FAST_PAIR_ADV_MODE_COUNT;

	if (adv_mode != BT_FAST_PAIR_ADV_MODE_DISC) {
		last_adv_mode = adv_mode;

		return false;
	}

	if (state->new_adv_session || (last_adv_mode != adv_mode)) {
		drop_disc_adv_payload = false;
		last_adv_mode = BT_FAST_PAIR_ADV_MODE_DISC;

		return false;
	}

	if (state->rpa_rotated) {
		drop_disc_adv_payload = true;
	}

	if (drop_disc_adv_payload) {
		LOG_WRN("RPA rotated during Fast Pair discoverable advertising session");
		LOG_WRN("Removed Fast Pair advertising payload");

		return true;
	}

	return false;
}

static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	static uint8_t buf[ADV_DATA_BUF_SIZE];
	struct bt_fast_pair_adv_config adv_config = { 0 };

	if (!enabled) {
		return -ENOENT;
	}

	if (!bt_fast_pair_is_ready()) {
		LOG_WRN("Fast Pair is not ready");
		return -ENOENT;
	}

	adv_config.mode = state->pairing_mode ? BT_FAST_PAIR_ADV_MODE_DISC :
						BT_FAST_PAIR_ADV_MODE_NOT_DISC;
	if (adv_config.mode == BT_FAST_PAIR_ADV_MODE_NOT_DISC) {
		adv_config.not_disc.type = show_ui_pairing ?
					   BT_FAST_PAIR_NOT_DISC_ADV_TYPE_SHOW_UI_IND :
					   BT_FAST_PAIR_NOT_DISC_ADV_TYPE_HIDE_UI_IND;
		adv_config.not_disc.battery_mode = adv_battery_mode;
	}

	if (IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_STOP_DISCOVERABLE_ON_RPA_ROTATION)) {
		if (detect_discoverable_stop_condition(adv_config.mode, state)) {
			return -ENOENT;
		}
	}

	if (IS_ENABLED(CONFIG_BT_ADV_PROV_FAST_PAIR_AUTO_SET_PAIRING_MODE)) {
		/* Set Fast Pair pairing mode to match Fast Pair advertising mode. */
		bt_fast_pair_set_pairing_mode(adv_config.mode == BT_FAST_PAIR_ADV_MODE_DISC);
	}

	__ASSERT_NO_MSG((bt_fast_pair_adv_data_size(adv_config) <= sizeof(buf)) &&
			(bt_fast_pair_adv_data_size(adv_config) > 0));

	return bt_fast_pair_adv_data_fill(ad, buf, sizeof(buf), adv_config);
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(fast_pair, get_data);
