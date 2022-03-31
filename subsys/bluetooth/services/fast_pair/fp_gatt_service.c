/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/gatt.h>

#include "fp_registration_data.h"
#include "fp_common.h"

/* Fast Pair GATT Service UUIDs defined by the Fast Pair specification. */
#define BT_UUID_FAST_PAIR		BT_UUID_DECLARE_16(BT_FAST_PAIR_SERVICE_UUID)
#define BT_UUID_FAST_PAIR_MODEL_ID \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1233, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))
#define BT_UUID_FAST_PAIR_KEY_BASED_PAIRING \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1234, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))
#define BT_UUID_FAST_PAIR_PASSKEY \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1235, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))
#define BT_UUID_FAST_PAIR_ACCOUNT_KEY \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xFE2C1236, 0x8366, 0x4814, 0x8EB0, 0x01DE32100BEA))

/* Make sure that MTU value of at least 83 is used (recommended by the Fast Pair specification). */
#define FP_RECOMMENDED_MTU	83
BUILD_ASSERT(BT_L2CAP_TX_MTU >= FP_RECOMMENDED_MTU);
BUILD_ASSERT(BT_L2CAP_RX_MTU >= FP_RECOMMENDED_MTU);


static ssize_t read_model_id(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	uint8_t model_id[FP_MODEL_ID_LEN];
	ssize_t res;

	if (!fp_get_model_id(model_id, sizeof(model_id))) {
		res = bt_gatt_attr_read(conn, attr, buf, len, offset, model_id, sizeof(model_id));
	} else {
		res = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return res;
}

static ssize_t write_key_based_pairing(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       const void *buf,
				       uint16_t len, uint16_t offset, uint8_t flags)
{
	/* Not yet supported. */
	return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
}

static ssize_t write_passkey(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	/* Not yet supported. */
	return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
}

static ssize_t write_account_key(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf,
				 uint16_t len, uint16_t offset, uint8_t flags)
{
	/* Not yet supported. */
	return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
}

BT_GATT_SERVICE_DEFINE(fast_pair_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_FAST_PAIR),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_MODEL_ID,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_model_id, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_KEY_BASED_PAIRING,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_key_based_pairing, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_PASSKEY,
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_WRITE,
		NULL, write_passkey, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_FAST_PAIR_ACCOUNT_KEY,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, write_account_key, NULL),
);
