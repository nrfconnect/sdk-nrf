/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_client.h>

LOG_MODULE_REGISTER(bt_latency_client, CONFIG_BT_LATENCY_CLIENT_LOG_LEVEL);

enum {
	LATENCY_INITIALIZED,
	LATENCY_ASYNC_WRITE_PENDING
};

static const struct bt_latency_client_cb *callbacks;

static void received_latency_response(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_latency_client *latency;
	const void *buf;
	uint16_t len;

	/* Retrieve Latency context. */
	latency = CONTAINER_OF(params, struct bt_latency_client,
			       latency_params);

	/* Make a copy of volatile data that is required by the callback. */
	buf = params->data;
	len = params->length;

	atomic_clear_bit(&latency->state, LATENCY_ASYNC_WRITE_PENDING);

	if (err) {
		LOG_ERR("Received invalid Latency response (err %d)", err);
		return;
	}

	LOG_DBG("Received Latency response, data %p length %u", buf, len);

	if (callbacks && callbacks->latency_response) {
		callbacks->latency_response(buf, len);
	}
}

int bt_latency_client_init(struct bt_latency_client *latency,
			   const struct bt_latency_client_cb *cb)
{
	if (!latency) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&latency->state, LATENCY_INITIALIZED)) {
		return -EALREADY;
	}

	callbacks = cb;
	return 0;
}

int bt_latency_handles_assign(struct bt_gatt_dm *dm,
			      struct bt_latency_client *latency)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_LATENCY)) {
		return -ENOTSUP;
	}

	LOG_DBG("Getting handles from Latency service.");

	/* Latency Characteristic. */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_LATENCY_CHAR);
	if (!gatt_chrc) {
		LOG_ERR("Missing Latency characteristic.");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_LATENCY_CHAR);
	if (!gatt_desc) {
		LOG_ERR("Missing Latency characteristic value descriptor.");
		return -EINVAL;
	}

	LOG_DBG("Found handle for Latency characteristic.");
	latency->handle = gatt_desc->handle;

	/* Prepare Latency parameters */
	latency->latency_params.func = received_latency_response;
	latency->latency_params.handle = latency->handle;

	/* Assign connection object. */
	latency->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

int bt_latency_request(struct bt_latency_client *latency,
		       const void *data, uint16_t len)
{
	int err;

	if (atomic_test_and_set_bit(&latency->state,
				    LATENCY_ASYNC_WRITE_PENDING)) {
		return -EALREADY;
	}

	LOG_DBG("Sending Latency request, data %p length %u", data, len);

	latency->latency_params.offset = 0;
	latency->latency_params.data = data;
	latency->latency_params.length = len;

	err = bt_gatt_write(latency->conn, &latency->latency_params);
	if (err) {
		LOG_ERR("Send Latency request failed (err %d)", err);
		atomic_clear_bit(&latency->state, LATENCY_ASYNC_WRITE_PENDING);
	}

	return err;
}
