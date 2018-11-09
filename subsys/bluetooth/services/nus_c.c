/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_c.h>

#define LOG_LEVEL CONFIG_BT_GATT_NUS_C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(nus_c);

enum {
	NUS_C_INITIALIZED,
	NUS_C_TX_NOTIF_ENABLED,
	NUS_C_RX_WRITE_PENDING
};

static u8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, u16_t length)
{
	struct bt_gatt_nus_c *nus_c;

	/* Retrieve NUS Client module context. */
	nus_c = CONTAINER_OF(params, struct bt_gatt_nus_c, tx_notif_params);

	if (!data) {
		LOG_DBG("[UNSUBSCRIBED]");
		params->value_handle = 0;
		atomic_clear_bit(&nus_c->state, NUS_C_TX_NOTIF_ENABLED);
		if (nus_c->cbs.tx_notif_disabled) {
			nus_c->cbs.tx_notif_disabled();
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[NOTIFICATION] data %p length %u", data, length);
	if (nus_c->cbs.data_received) {
		return nus_c->cbs.data_received(data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void on_sent(struct bt_conn *conn, u8_t err,
		    struct bt_gatt_write_params *params)
{
	struct bt_gatt_nus_c *nus_c;
	const void *data;
	u16_t length;

	/* Retrieve NUS Client module context. */
	nus_c = CONTAINER_OF(params, struct bt_gatt_nus_c, rx_write_params);

	/* Make a copy of volatile data that is required by the callback. */
	data = params->data;
	length = params->length;

	atomic_clear_bit(&nus_c->state, NUS_C_RX_WRITE_PENDING);
	if (nus_c->cbs.data_sent) {
		nus_c->cbs.data_sent(err, data, length);
	}
}

int bt_gatt_nus_c_init(struct bt_gatt_nus_c *nus_c,
		       const struct bt_gatt_nus_c_init_param *nus_c_init)
{
	if (!nus_c || !nus_c_init) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&nus_c->state, NUS_C_INITIALIZED)) {
		return -EALREADY;
	}

	memcpy(&nus_c->cbs, &nus_c_init->cbs, sizeof(nus_c->cbs));

	return 0;
}

int bt_gatt_nus_c_send(struct bt_gatt_nus_c *nus_c, const u8_t *data,
		       u16_t len)
{
	int err;

	if (atomic_test_and_set_bit(&nus_c->state, NUS_C_RX_WRITE_PENDING)) {
		return -EALREADY;
	}

	nus_c->rx_write_params.func = on_sent;
	nus_c->rx_write_params.handle = nus_c->handles.rx;
	nus_c->rx_write_params.offset = 0;
	nus_c->rx_write_params.data = data;
	nus_c->rx_write_params.length = len;

	err = bt_gatt_write(nus_c->conn, &nus_c->rx_write_params);
	if (err) {
		atomic_clear_bit(&nus_c->state, NUS_C_RX_WRITE_PENDING);
	}

	return err;
}

int bt_gatt_nus_c_handles_assign(struct bt_gatt_nus_c *nus_c,
				 struct bt_conn *conn,
				 const struct bt_gatt_attr *attrs,
				 size_t attrs_len)
{
	struct bt_gatt_service_val *gatt_service = attrs->user_data;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_NUS_SERVICE)) {
		return -EINVAL;
	}
	LOG_DBG("Getting handles from NUS service.");

	/* Assign handles required by NUS. */
	memset(&nus_c->handles, 0xFF, sizeof(nus_c->handles));
	for (size_t i = 0; i < attrs_len; i++) {
		if (!bt_uuid_cmp(attrs[i].uuid, BT_UUID_NUS_TX)) {
			nus_c->handles.tx = attrs[i].handle;
			LOG_DBG("Found handle for NUS TX Characteristic.");
			if (!bt_uuid_cmp(attrs[i + 1].uuid, BT_UUID_GATT_CCC)) {
				nus_c->handles.tx_ccc = attrs[i + 1].handle;
				LOG_DBG("Found handle for CCC of NUS TX"
					" characteristic.");
			}
		} else if (!bt_uuid_cmp(attrs[i].uuid, BT_UUID_NUS_RX)) {
			nus_c->handles.rx = attrs[i].handle;
			LOG_DBG("Found handle for NUS RX Characteristic");
		}
	}

	/* Validate if required handles are present. */
	u16_t *handle;

	handle = (u16_t *) &nus_c->handles;
	for (size_t i = 0; i < sizeof(nus_c->handles) / sizeof(u16_t); i++) {
		if (handle[i] == 0xffff) {
			LOG_ERR("All required handles need to be set.");
			return -EINVAL;
		}
	}

	/* Assign connection instance. */
	nus_c->conn = conn;
	return 0;
}

int bt_gatt_nus_c_tx_notif_enable(struct bt_gatt_nus_c *nus_c)
{
	int err;

	if (atomic_test_and_set_bit(&nus_c->state, NUS_C_TX_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	nus_c->tx_notif_params.notify = on_received;
	nus_c->tx_notif_params.value = BT_GATT_CCC_NOTIFY;
	nus_c->tx_notif_params.value_handle = nus_c->handles.tx;
	nus_c->tx_notif_params.ccc_handle = nus_c->handles.tx_ccc;

	err = bt_gatt_subscribe(nus_c->conn, &nus_c->tx_notif_params);
	if (err) {
		LOG_ERR("Subscribe failed (err %d)", err);
		atomic_clear_bit(&nus_c->state, NUS_C_TX_NOTIF_ENABLED);
	} else {
		LOG_DBG("[SUBSCRIBED]");
	}

	return err;
}
