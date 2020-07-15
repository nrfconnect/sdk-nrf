/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "dev_descr.h"

static u8_t device_descr[] = {
	[DEV_DESCR_LLPM_SUPPORT_POS] = IS_ENABLED(CONFIG_DESKTOP_BLE_USE_LLPM),
};

static ssize_t read_dev_descr(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, u16_t len, u16_t offset)
{
	__ASSERT_NO_MSG(attr != NULL);
	__ASSERT_NO_MSG(attr->user_data != NULL);
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(device_descr));
}

/* Custom GATT service - device description */
BT_GATT_SERVICE_DEFINE(custom_desc_svc,
	BT_GATT_PRIMARY_SERVICE(&custom_desc_uuid),
	BT_GATT_CHARACTERISTIC(&custom_desc_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_dev_descr, NULL, &device_descr),
);
