/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <bluetooth/adv_prov.h>


static int get_data(struct bt_data *d, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(fb);

	if (!state->pairing_mode) {
		return -ENOENT;
	}

	static uint8_t data[sizeof(uint16_t)];

	sys_put_le16(bt_get_appearance(), data);

	d->type = BT_DATA_GAP_APPEARANCE;
	d->data_len = sizeof(data);
	d->data = data;

	return 0;
}

#if CONFIG_BT_ADV_PROV_GAP_APPEARANCE_SD
BT_LE_ADV_PROV_SD_PROVIDER_REGISTER(gap_appearance, get_data);
#else
BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(gap_appearance, get_data);
#endif /* CONFIG_BT_ADV_PROV_GAP_APPEARANCE_SD */
