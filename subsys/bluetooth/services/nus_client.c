/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nus_c, CONFIG_BT_NUS_CLIENT_LOG_LEVEL);

enum {
	NUS_C_INITIALIZED,
	NUS_C_TX_NOTIF_ENABLED,
	NUS_C_RX_WRITE_PENDING
};

static uint8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	struct bt_nus_client *nus;

	/* Retrieve NUS Client module context. */
	nus = CONTAINER_OF(params, struct bt_nus_client, tx_notif_params);

	if (!data) {
		LOG_DBG("[UNSUBSCRIBED]");
		params->value_handle = 0;
		atomic_clear_bit(&nus->state, NUS_C_TX_NOTIF_ENABLED);
		if (nus->cb.unsubscribed) {
			nus->cb.unsubscribed(nus);
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[NOTIFICATION] data %p length %u", data, length);
	if (nus->cb.received) {
		return nus->cb.received(nus, data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void on_sent(struct bt_conn *conn, uint8_t err,
		    struct bt_gatt_write_params *params)
{
	struct bt_nus_client *nus_c;
	const void *data;
	uint16_t length;

	/* Retrieve NUS Client module context. */
	nus_c = CONTAINER_OF(params, struct bt_nus_client, rx_write_params);

	/* Make a copy of volatile data that is required by the callback. */
	data = params->data;
	length = params->length;

	atomic_clear_bit(&nus_c->state, NUS_C_RX_WRITE_PENDING);
	if (nus_c->cb.sent) {
		nus_c->cb.sent(nus_c, err, data, length);
	}
}

int bt_nus_client_init(struct bt_nus_client *nus_c,
		       const struct bt_nus_client_init_param *nus_c_init)
{
	if (!nus_c || !nus_c_init) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&nus_c->state, NUS_C_INITIALIZED)) {
		return -EALREADY;
	}

	memcpy(&nus_c->cb, &nus_c_init->cb, sizeof(nus_c->cb));

	return 0;
}

int bt_nus_client_send(struct bt_nus_client *nus_c, const uint8_t *data,
		       uint16_t len)
{
	int err;

	if (!nus_c->conn) {
		return -ENOTCONN;
	}

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

int bt_nus_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_nus_client *nus_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_NUS_SERVICE)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from NUS service.");
	memset(&nus_c->handles, 0xFF, sizeof(nus_c->handles));

	/* NUS TX Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_NUS_TX);
	if (!gatt_chrc) {
		LOG_ERR("Missing NUS TX characteristic.");
		return -EINVAL;
	}
	/* NUS TX */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_NUS_TX);
	if (!gatt_desc) {
		LOG_ERR("Missing NUS TX value descriptor in characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for NUS TX characteristic.");
	nus_c->handles.tx = gatt_desc->handle;
	/* NUS TX CCC */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_ERR("Missing NUS TX CCC in characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for CCC of NUS TX characteristic.");
	nus_c->handles.tx_ccc = gatt_desc->handle;

	/* NUS RX Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_NUS_RX);
	if (!gatt_chrc) {
		LOG_ERR("Missing NUS RX characteristic.");
		return -EINVAL;
	}
	/* NUS RX */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_NUS_RX);
	if (!gatt_desc) {
		LOG_ERR("Missing NUS RX value descriptor in characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for NUS RX characteristic.");
	nus_c->handles.rx = gatt_desc->handle;

	/* Assign connection instance. */
	nus_c->conn = bt_gatt_dm_conn_get(dm);
	return 0;
}

int bt_nus_subscribe_receive(struct bt_nus_client *nus_c)
{
	int err;

	if (atomic_test_and_set_bit(&nus_c->state, NUS_C_TX_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	nus_c->tx_notif_params.notify = on_received;
	nus_c->tx_notif_params.value = BT_GATT_CCC_NOTIFY;
	nus_c->tx_notif_params.value_handle = nus_c->handles.tx;
	nus_c->tx_notif_params.ccc_handle = nus_c->handles.tx_ccc;
	atomic_set_bit(nus_c->tx_notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(nus_c->conn, &nus_c->tx_notif_params);
	if (err) {
		LOG_ERR("Subscribe failed (err %d)", err);
		atomic_clear_bit(&nus_c->state, NUS_C_TX_NOTIF_ENABLED);
	} else {
		LOG_DBG("[SUBSCRIBED]");
	}

	return err;
}
