/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <kernel.h>
#include <sys/printk.h>
#include <string.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <bluetooth/services/throughput.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_gatt_throughput, CONFIG_BT_GATT_THROUGHPUT_LOG_LEVEL);

static struct bt_gatt_throughput_metrics met;
static const struct bt_gatt_throughput_cb *callbacks;

static u8_t read_fn(struct bt_conn *conn, u8_t err,
		    struct bt_gatt_read_params *params, const void *data,
		    u16_t len)
{
	struct bt_gatt_throughput_metrics metrics;

	memset(&metrics, 0, sizeof(struct bt_gatt_throughput_metrics));

	if (data) {
		len = MIN(len, sizeof(struct bt_gatt_throughput_metrics));
		memcpy(&metrics, data, len);

		if (callbacks->data_read) {
			return callbacks->data_read(&metrics);
		}
	}

	return BT_GATT_ITER_STOP;
}

static ssize_t write_callback(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      u16_t len, u16_t offset, u8_t flags)
{
	static u32_t clock_cycles;
	static u32_t kb;

	u64_t delta;

	struct bt_gatt_throughput_metrics *met_data = attr->user_data;

	delta = k_cycle_get_32() - clock_cycles;
	delta = k_cyc_to_ns_floor64(delta);

	if (len == 1) {
		/* reset metrics */
		kb = 0;
		met_data->write_count = 0;
		met_data->write_len = 0;
		met_data->write_rate = 0;
		clock_cycles = k_cycle_get_32();
	} else {
		met_data->write_count++;
		met_data->write_len += len;
		met_data->write_rate =
		    ((u64_t)met_data->write_len << 3) * 1000000000 / delta;
	}

	LOG_DBG("Received data.");

	if (callbacks->data_received) {
		callbacks->data_received(met_data);
	}

	return len;
}

static ssize_t read_callback(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	const struct bt_gatt_throughput_metrics *metrics = attr->user_data;

	len = MIN(sizeof(struct bt_gatt_throughput_metrics), len);

	if (callbacks->data_send) {
		callbacks->data_send(metrics);
	}

	LOG_DBG("Data send.");

	return bt_gatt_attr_read(
		conn, attr, buf, len, offset, attr->user_data, len);
}


BT_GATT_SERVICE_DEFINE(throughput_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_THROUGHPUT),
	BT_GATT_CHARACTERISTIC(BT_UUID_THROUGHPUT_CHAR,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_callback, write_callback, &met),
);

int bt_gatt_throughput_init(struct bt_gatt_throughput *throughput,
			    const struct bt_gatt_throughput_cb *cb)
{
	if (!throughput || !cb) {
		return -EINVAL;
	}

	callbacks = cb;

	return 0;
}

int bt_gatt_throughput_handles_assign(struct bt_gatt_dm *dm,
				      struct bt_gatt_throughput *throughput)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_THROUGHPUT)) {
		return -ENOTSUP;
	}

	LOG_DBG("Getting handles from Throughput service.");

	/* Throughput Characteristic. */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_THROUGHPUT_CHAR);
	if (!gatt_chrc) {
		LOG_ERR("Missing Throughput characteristic.");
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_THROUGHPUT_CHAR);
	if (!gatt_desc) {
		LOG_ERR("Missing Throughput characteristic value descriptor");
		return -EINVAL;
	}

	LOG_DBG("Found handle for Throughput characteristic.");
	throughput->char_handle = gatt_desc->handle;

	/* Assign connection object. */
	throughput->conn = bt_gatt_dm_conn_get(dm);
	return 0;
}

int bt_gatt_throughput_read(struct bt_gatt_throughput *throughput)
{
	int err;

	throughput->read_params.single.handle = throughput->char_handle;
	throughput->read_params.single.offset = 0;
	throughput->read_params.handle_count = 1;
	throughput->read_params.func = read_fn;

	err = bt_gatt_read(throughput->conn, &throughput->read_params);
	if (err) {
		LOG_ERR("Characteristic read failed.");
	}

	return err;
}

int bt_gatt_throughput_write(struct bt_gatt_throughput *throughput,
			     const u8_t *data, u16_t len)
{
	return bt_gatt_write_without_response(throughput->conn,
					      throughput->char_handle,
					      data, len, false);
}
