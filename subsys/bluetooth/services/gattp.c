/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/gattp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gattp, CONFIG_BT_GATTP_LOG_LEVEL);

/** Length of the Service Changed characteristic value. */
#define BT_GATTP_SERVICE_CHANGED_VALUE_LENGTH 4

enum {
	GATTP_SC_INDICATE_ENABLED
};

int bt_gattp_init(struct bt_gattp *gattp)
{
	if (!gattp) {
		return -EINVAL;
	}

	memset(gattp, 0, sizeof(struct bt_gattp));

	return 0;
}

static void gattp_reinit(struct bt_gattp *gattp)
{
	gattp->conn = NULL;
	gattp->handle_sc = 0;
	gattp->handle_sc_ccc = 0;
	gattp->state = ATOMIC_INIT(0);
}

int bt_gattp_handles_assign(struct bt_gatt_dm *dm,
			    struct bt_gattp *gattp)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_GATT)) {
		return -ENOTSUP;
	}

	LOG_DBG("GATT Service found");

	gattp_reinit(gattp);

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_GATT_SC);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_GATT_SC);
	if (!gatt_desc) {
		return -EINVAL;
	}

	gattp->handle_sc = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}

	gattp->handle_sc_ccc = gatt_desc->handle;

	LOG_DBG("Service Changed characteristic found.");

	/* Finally - save connection object */
	gattp->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

static uint8_t bt_gattp_sc_indicate_callback(struct bt_conn *conn,
					     struct bt_gatt_subscribe_params *params,
					     const void *data, uint16_t length)
{
	struct bt_gattp *gattp;
	struct bt_gattp_handle_range handle_range = {0};
	int err;
	const uint8_t *handles_data = (const uint8_t *)data;

	/* Retrieve GATT Service client module context. */
	gattp = CONTAINER_OF(params, struct bt_gattp, indicate_params);

	if (gattp->indicate_cb) {
		err = (length == BT_GATTP_SERVICE_CHANGED_VALUE_LENGTH) ? 0 : -EINVAL;

		if (!err) {
			handle_range.start_handle = sys_get_le16(handles_data);
			handle_range.end_handle = sys_get_le16(handles_data + 2);
		}

		gattp->indicate_cb(gattp, &handle_range, err);
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_gattp_subscribe_service_changed(struct bt_gattp *gattp,
				       bt_gattp_indicate_cb func)
{
	int err;

	if (!gattp || !gattp->conn || !func) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&gattp->state, GATTP_SC_INDICATE_ENABLED)) {
		return -EALREADY;
	}

	gattp->indicate_cb = func;

	gattp->indicate_params.notify = bt_gattp_sc_indicate_callback;
	gattp->indicate_params.value = BT_GATT_CCC_INDICATE;
	gattp->indicate_params.value_handle = gattp->handle_sc;
	gattp->indicate_params.ccc_handle = gattp->handle_sc_ccc;
	atomic_set_bit(gattp->indicate_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(gattp->conn, &gattp->indicate_params);
	if (err) {
		atomic_clear_bit(&gattp->state, GATTP_SC_INDICATE_ENABLED);
		LOG_ERR("Subscribe Service Changed failed (err %d)", err);
	} else {
		LOG_DBG("Service Changed subscribed");
	}

	return err;
}

int bt_gattp_unsubscribe_service_changed(struct bt_gattp *gattp)
{
	int err;

	if (!gattp) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&gattp->state, GATTP_SC_INDICATE_ENABLED)) {
		return -EALREADY;
	}

	err = bt_gatt_unsubscribe(gattp->conn, &gattp->indicate_params);
	if (err) {
		LOG_ERR("Unsubscribe Service Changed failed (err %d)", err);
	} else {
		atomic_clear_bit(&gattp->state, GATTP_SC_INDICATE_ENABLED);
		LOG_DBG("Service Changed unsubscribed");
	}

	return err;
}
