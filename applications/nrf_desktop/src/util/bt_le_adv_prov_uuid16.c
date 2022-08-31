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

	if (state->bond_cnt > 0) {
		return -ENOENT;
	}

	static const uint8_t data[] = {
#if CONFIG_DESKTOP_HIDS_ENABLE
		0x12, 0x18,   /* HID Service */
#endif
#if CONFIG_DESKTOP_BAS_ENABLE
		0x0f, 0x18,   /* Battery Service */
#endif
	};

	ad->type = BT_DATA_UUID16_ALL;
	ad->data_len = sizeof(data);
	ad->data = data;

	return 0;
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(uuid16_all, get_data);
