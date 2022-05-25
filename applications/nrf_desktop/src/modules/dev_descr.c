/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/drivers/hwinfo.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include "hwid.h"
#include "dev_descr.h"

static uint8_t device_descr[] = {
	[DEV_DESCR_LLPM_SUPPORT_POS] = IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM),
};

static ssize_t read_hwid(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	__ASSERT_NO_MSG(attr != NULL);

	uint8_t hwid[HWID_LEN];

	hwid_get(hwid, sizeof(hwid));

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 hwid, sizeof(hwid));
}

static ssize_t read_dev_descr(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint16_t len, uint16_t offset)
{
	__ASSERT_NO_MSG(attr != NULL);
	__ASSERT_NO_MSG(attr->user_data != NULL);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, device_descr,
				 sizeof(device_descr));
}

/* Custom GATT service - device description */
BT_GATT_SERVICE_DEFINE(custom_desc_svc,
	BT_GATT_PRIMARY_SERVICE(&custom_desc_uuid),
	BT_GATT_CHARACTERISTIC(&custom_desc_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_dev_descr, NULL, device_descr),
	BT_GATT_CHARACTERISTIC(&hwid_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_hwid, NULL, NULL),
);
