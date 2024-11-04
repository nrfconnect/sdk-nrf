/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <bluetooth/services/ras.h>

LOG_MODULE_REGISTER(ras_rrsp, CONFIG_BT_RAS_RRSP_LOG_LEVEL);

static uint32_t ras_features; /* No optional features supported. */

static ssize_t ras_features_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ras_features, sizeof(ras_features));
}

BT_GATT_SERVICE_DEFINE(rrsp_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_RANGING_SERVICE),
	/* RAS Features */
	BT_GATT_CHARACTERISTIC(BT_UUID_RAS_FEATURES, BT_GATT_CHRC_READ,
				BT_GATT_PERM_READ_ENCRYPT, ras_features_read, NULL, NULL),
);
