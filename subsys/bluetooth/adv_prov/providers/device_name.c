/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <bluetooth/adv_prov.h>


static int get_data(struct bt_data *sd, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(state);
	ARG_UNUSED(fb);

	const char *name = bt_get_name();

	sd->type = BT_DATA_NAME_COMPLETE;
	sd->data_len = strlen(name);
	sd->data = name;

	return 0;
}

BT_LE_ADV_PROV_SD_PROVIDER_REGISTER(device_name, get_data);
