/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/nus.h>
#include <bluetooth/adv_prov.h>

static const struct bt_data data = BT_DATA_BYTES(BT_DATA_UUID128_ALL,
						 BT_UUID_NUS_VAL);


static int get_data(struct bt_data *sd, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(fb);

	if (state->bond_cnt > 0) {
		return -ENOENT;
	}

	*sd = data;

	return 0;
}

BT_LE_ADV_PROV_SD_PROVIDER_REGISTER(uuid128_all, get_data);
