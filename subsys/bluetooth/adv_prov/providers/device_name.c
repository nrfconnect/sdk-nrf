/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <bluetooth/adv_prov.h>


static int get_data(struct bt_data *d, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(fb);

	if (IS_ENABLED(CONFIG_BT_ADV_PROV_DEVICE_NAME_PAIRING_MODE_ONLY) &&
	    !state->pairing_mode) {
		return -ENOENT;
	}

	const char *name = bt_get_name();

	d->type = BT_DATA_NAME_COMPLETE;
	d->data_len = strlen(name);
	d->data = name;

	return 0;
}

#if CONFIG_BT_ADV_PROV_DEVICE_NAME_SD
BT_LE_ADV_PROV_SD_PROVIDER_REGISTER(device_name, get_data);
#else
BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(device_name, get_data);
#endif /* CONFIG_BT_ADV_PROV_DEVICE_NAME_SD */
