/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov.h>
#include <bluetooth/adv_prov/swift_pair.h>

#define GRACE_PERIOD_S	CONFIG_BT_ADV_PROV_SWIFT_PAIR_COOL_DOWN_DURATION

static bool enabled = true;


void bt_le_adv_prov_swift_pair_enable(bool enable)
{
	enabled = enable;
}

static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	static const uint8_t data[] = {
		0x06, 0x00,	/* Microsoft Vendor ID */
		0x03,		/* Microsoft Beacon ID */
		0x00,		/* Microsoft Beacon Sub Scenario */
		0x80		/* Reserved RSSI Byte */
	};

	if (!enabled) {
		return -ENOENT;
	}

	if (state->in_grace_period || (!state->pairing_mode)) {
		return -ENOENT;
	}

	ad->type = BT_DATA_MANUFACTURER_DATA;
	ad->data_len = sizeof(data);
	ad->data = data;

	fb->grace_period_s = GRACE_PERIOD_S;

	return 0;
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(swift_pair, get_data);
