/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov.h>


static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(fb);

	static uint8_t flags = BT_LE_AD_NO_BREDR;

	if (state->pairing_mode) {
		flags |= BT_LE_AD_GENERAL;
	} else {
		flags &= ~BT_LE_AD_GENERAL;
	}

	ad->type = BT_DATA_FLAGS;
	ad->data_len = sizeof(flags);
	ad->data = &flags;

	return 0;
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(flags, get_data);
