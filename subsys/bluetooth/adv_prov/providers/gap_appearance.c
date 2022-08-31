/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <bluetooth/adv_prov.h>


static int get_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(fb);

	if (state->bond_cnt > 0) {
		return -ENOENT;
	}

	static uint8_t data[sizeof(uint16_t)];

	sys_put_le16(bt_get_appearance(), data);

	ad->type = BT_DATA_GAP_APPEARANCE;
	ad->data_len = sizeof(data);
	ad->data = data;

	return 0;
}

BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(gap_appearance, get_data);
