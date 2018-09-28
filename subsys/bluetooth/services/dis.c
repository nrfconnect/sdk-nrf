/** @file
 *  @brief DIS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/dis.h>


#define DIS_PNP_ID_LEN 7 /**< Length of PnP ID Characteristic Value */

/** The pointer of the model name */
static const char dis_model[] = CONFIG_NRF_BT_DIS_MODEL;
/** The pointer of manufacturer name */
static const char dis_manuf[] = CONFIG_NRF_BT_DIS_MANUFACTURER;
/** PnP ID coded data */
#if CONFIG_NRF_BT_PNP_CHAR
static const u8_t dis_pnp_id[DIS_PNP_ID_LEN] = {
	CONFIG_NRF_BT_PNP_VENDOR_ID_SOURCE,
	(CONFIG_NRF_BT_PNP_VENDOR_ID >> 0) & 0xFF,
	(CONFIG_NRF_BT_PNP_VENDOR_ID >> 8) & 0xFF,
	(CONFIG_NRF_BT_PNP_PRODUCT_ID >> 0) & 0xFF,
	(CONFIG_NRF_BT_PNP_PRODUCT_ID >> 8) & 0xFF,
	(CONFIG_NRF_BT_PNP_PRODUCT_VER >> 0) & 0xFF,
	(CONFIG_NRF_BT_PNP_PRODUCT_VER >> 8) & 0xFF
};
#else
#if CONFIG_NRF_BT_HIDS
#warning "PnP_ID characteristic is mandatory if HID Service is used"
#endif
#endif


static ssize_t read_model(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_model,
				 strlen(dis_model));
}

static ssize_t read_manuf(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_manuf,
				 strlen(dis_manuf));
}

#if CONFIG_NRF_BT_PNP_CHAR
static ssize_t read_pnpid(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_pnp_id,
				 sizeof(dis_pnp_id));
}
#endif


/* Device Information Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_model, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_manuf, NULL, NULL),
#if CONFIG_NRF_BT_PNP_CHAR
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_PNP_ID, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_pnpid, NULL, NULL),
#endif
};

static struct bt_gatt_service dis_svc = BT_GATT_SERVICE(attrs);

void dis_init(void)
{
	bt_gatt_service_register(&dis_svc);
}
