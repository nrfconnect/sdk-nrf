/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/bas_client.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bas_client, CONFIG_BT_BAS_CLIENT_LOG_LEVEL);

/**
 * @brief Process battery level value notification
 *
 * Internal function to process report notification and pass it further.
 *
 * @param conn   Connection handler.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to subscribe function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static uint8_t notify_process(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	struct bt_bas_client *bas;
	uint8_t battery_level;
	const uint8_t *bdata = data;

	bas = CONTAINER_OF(params, struct bt_bas_client, notify_params);
	if (!data || !length) {
		LOG_INF("Notifications disabled.");
		if (bas->notify_cb) {
			bas->notify_cb(bas, BT_BAS_VAL_INVALID);
		}
		return BT_GATT_ITER_STOP;
	}
	if (length != 1) {
		LOG_ERR("Unexpected notification value size.");
		if (bas->notify_cb) {
			bas->notify_cb(bas, BT_BAS_VAL_INVALID);
		}
		return BT_GATT_ITER_STOP;
	}

	battery_level = bdata[0];
	if (battery_level > BT_BAS_VAL_MAX) {
		LOG_ERR("Unexpected notification value.");
		if (bas->notify_cb) {
			bas->notify_cb(bas, BT_BAS_VAL_INVALID);
		}
		return BT_GATT_ITER_STOP;
	}
	bas->battery_level = battery_level;
	if (bas->notify_cb) {
		bas->notify_cb(bas, battery_level);
	}

	return BT_GATT_ITER_CONTINUE;
}

/**
 * @brief Process battery level value read
 *
 * Internal function to process report read and pass it further.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static uint8_t read_process(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_read_params *params,
			     const void *data, uint16_t length)
{
	struct bt_bas_client *bas;
	uint8_t battery_level = BT_BAS_VAL_INVALID;
	const uint8_t *bdata = data;

	bas = CONTAINER_OF(params, struct bt_bas_client, read_params);

	if (!bas->read_cb) {
		LOG_ERR("No read callback present");
	} else  if (err) {
		LOG_ERR("Read value error: %d", err);
		bas->read_cb(bas,  battery_level, err);
	} else if (!data || length != 1) {
		bas->read_cb(bas,  battery_level, -EMSGSIZE);
	} else {
		battery_level = bdata[0];
		if (battery_level > BT_BAS_VAL_MAX) {
			LOG_ERR("Unexpected read value.");
			bas->read_cb(bas, BT_BAS_VAL_INVALID, err);
		} else {
			bas->battery_level = battery_level;
			bas->read_cb(bas, battery_level, err);
		}
	}

	bas->read_cb = NULL;

	return BT_GATT_ITER_STOP;
}

/**
 * @brief Process periodic battery level value read
 *
 * Internal function to process report read and pass it further.
 * And the end the new read request is triggered.
 *
 * @param conn   Connection handler.
 * @param err    Read ATT error code.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to read function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static uint8_t periodic_read_process(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	int32_t interval;
	struct bt_bas_client *bas;
	uint8_t battery_level = BT_BAS_VAL_INVALID;
	const uint8_t *bdata = data;

	bas = CONTAINER_OF(params, struct bt_bas_client,
			periodic_read.params);

	if (!bas->notify_cb) {
		LOG_ERR("No notification callback present");
	} else  if (err) {
		LOG_ERR("Read value error: %d", err);
	} else if (!data || length != 1) {
		LOG_ERR("Unexpected read value size.");
	} else {
		battery_level = bdata[0];
		if (battery_level > BT_BAS_VAL_MAX) {
			LOG_ERR("Unexpected read value.");
		} else if (bas->battery_level != battery_level) {
			bas->battery_level = battery_level;
			bas->notify_cb(bas, battery_level);
		} else {
			/* Do nothing. */
		}
	}

	interval = atomic_get(&bas->periodic_read.interval);
	if (interval) {
		k_work_schedule(&bas->periodic_read.read_work,
				K_MSEC(interval));
	}
	return BT_GATT_ITER_STOP;
}


/**
 * @brief Periodic read workqueue handler.
 *
 * @param work Work instance.
 */
static void bas_read_value_handler(struct k_work *work)
{
	int err;
	struct bt_bas_client *bas;

	bas = CONTAINER_OF(work, struct bt_bas_client,
			     periodic_read.read_work.work);

	if (!atomic_get(&bas->periodic_read.interval)) {
		/* disabled */
		return;
	}

	if (!bas->conn) {
		LOG_ERR("No connection object.");
		return;
	}

	bas->periodic_read.params.func = periodic_read_process;
	bas->periodic_read.params.handle_count  = 1;
	bas->periodic_read.params.single.handle = bas->val_handle;
	bas->periodic_read.params.single.offset = 0;

	err = bt_gatt_read(bas->conn, &bas->periodic_read.params);

	/* Do not treats reading after disconnection as error.
	 * Periodic read process is stopped after disconnection.
	 */
	if (err) {
		LOG_ERR("Periodic Battery Level characteristic read error: %d",
			err);
	}
}


/**
 * @brief Reinitialize the BAS Client.
 *
 * @param bas BAS Client object.
 */
static void bas_reinit(struct bt_bas_client *bas)
{
	bas->ccc_handle = 0;
	bas->val_handle = 0;
	bas->battery_level = BT_BAS_VAL_INVALID;
	bas->conn = NULL;
	bas->notify_cb = NULL;
	bas->read_cb = NULL;
	bas->notify = false;
}


void bt_bas_client_init(struct bt_bas_client *bas)
{
	memset(bas, 0, sizeof(*bas));
	bas->battery_level = BT_BAS_VAL_INVALID;

	k_work_init_delayable(&bas->periodic_read.read_work,
			      bas_read_value_handler);
}


int bt_bas_handles_assign(struct bt_gatt_dm *dm,
				 struct bt_bas_client *bas)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;
	const struct bt_gatt_chrc *chrc_val;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_BAS)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from battery service.");

	/* If connection is established again, cancel previous read request. */
	k_work_cancel_delayable(&bas->periodic_read.read_work);
	/* When workqueue is used its instance cannont be cleared. */
	bas_reinit(bas);

	/* Battery level characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_BAS_BATTERY_LEVEL);
	if (!gatt_chrc) {
		LOG_ERR("No battery level characteristic found.");
		return -EINVAL;
	}
	chrc_val = bt_gatt_dm_attr_chrc_val(gatt_chrc);
	__ASSERT_NO_MSG(chrc_val); /* This is internal function and it has to
				    * be called with characteristic attribute
				    */
	bas->properties = chrc_val->properties;
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_BAS_BATTERY_LEVEL);
	if (!gatt_desc) {
		LOG_ERR("No battery level characteristic value found.");
		return -EINVAL;
	}
	bas->val_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_INF("No battery CCC descriptor found. Battery service do not supported notification.");
	} else {
		bas->notify = true;
		bas->ccc_handle = gatt_desc->handle;
	}

	/* Finally - save connection object */
	bas->conn = bt_gatt_dm_conn_get(dm);
	return 0;
}

int bt_bas_subscribe_battery_level(struct bt_bas_client *bas,
				   bt_bas_notify_cb func)
{
	int err;

	if (!bas || !func) {
		return -EINVAL;
	}
	if (!bas->conn) {
		return -EINVAL;
	}
	if (!(bas->properties & BT_GATT_CHRC_NOTIFY)) {
		return -ENOTSUP;
	}
	if (bas->notify_cb) {
		return -EALREADY;
	}

	bas->notify_cb = func;

	bas->notify_params.notify = notify_process;
	bas->notify_params.value = BT_GATT_CCC_NOTIFY;
	bas->notify_params.value_handle = bas->val_handle;
	bas->notify_params.ccc_handle = bas->ccc_handle;
	atomic_set_bit(bas->notify_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	LOG_DBG("Subscribe: val: %u, ccc: %u",
		bas->notify_params.value_handle,
		bas->notify_params.ccc_handle);
	err = bt_gatt_subscribe(bas->conn, &bas->notify_params);
	if (err) {
		LOG_ERR("Report notification subscribe error: %d.", err);
		bas->notify_cb = NULL;
		return err;
	}
	LOG_DBG("Report subscribed.");
	return err;
}


int bt_bas_unsubscribe_battery_level(struct bt_bas_client *bas)
{
	int err;

	if (!bas) {
		return -EINVAL;
	}

	if (!bas->notify_cb) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(bas->conn, &bas->notify_params);
	bas->notify_cb = NULL;
	return err;
}


struct bt_conn *bt_bas_conn(const struct bt_bas_client *bas)
{
	return bas->conn;
}


int bt_bas_read_battery_level(struct bt_bas_client *bas, bt_bas_read_cb func)
{
	int err;

	if (!bas || !func) {
		return -EINVAL;
	}
	if (!bas->conn) {
		return -EINVAL;
	}
	if (bas->read_cb) {
		return -EBUSY;
	}
	bas->read_cb = func;
	bas->read_params.func = read_process;
	bas->read_params.handle_count  = 1;
	bas->read_params.single.handle = bas->val_handle;
	bas->read_params.single.offset = 0;

	err = bt_gatt_read(bas->conn, &bas->read_params);
	if (err) {
		bas->read_cb = NULL;
		return err;
	}
	return 0;
}


int bt_bas_get_last_battery_level(struct bt_bas_client *bas)
{
	if (!bas) {
		return -EINVAL;
	}

	return bas->battery_level;
}


int bt_bas_start_per_read_battery_level(struct bt_bas_client *bas,
					int32_t interval,
					bt_bas_notify_cb func)
{
	if (!bas || !func || !interval) {
		return -EINVAL;
	}

	if (bt_bas_notify_supported(bas)) {
		return -ENOTSUP;
	}

	bas->notify_cb = func;
	atomic_set(&bas->periodic_read.interval, interval);
	k_work_schedule(&bas->periodic_read.read_work, K_MSEC(interval));

	return 0;
}


void bt_bas_stop_per_read_battery_level(struct bt_bas_client *bas)
{
	/* If read is proccesed now, prevent triggering new
	 * characteristic read.
	 */
	atomic_set(&bas->periodic_read.interval, 0);

	/* If delayed workqueue pending, cancel it. If this fails, we'll exit
	 * early in the read handler due to the interval.
	 */
	k_work_cancel_delayable(&bas->periodic_read.read_work);
}
