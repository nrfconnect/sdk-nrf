/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <kernel.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/bas_c.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bas_c, CONFIG_BT_GATT_BAS_C_LOG_LEVEL);


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
static u8_t notify_process(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	struct bt_gatt_bas_c *bas_c;
	u8_t battery_level;
	const u8_t *bdata = data;

	bas_c = CONTAINER_OF(params, struct bt_gatt_bas_c, notify_params);
	if (!data || !length) {
		LOG_ERR("NULL notification received.");
		if (bas_c->notify_cb) {
			bas_c->notify_cb(bas_c, BT_GATT_BAS_VAL_INVALID);
		}
		return BT_GATT_ITER_STOP;
	}
	if (length != 1) {
		LOG_ERR("Unexpected notification value size.");
		if (bas_c->notify_cb) {
			bas_c->notify_cb(bas_c, BT_GATT_BAS_VAL_INVALID);
		}
		return BT_GATT_ITER_STOP;
	}

	battery_level = bdata[0];
	if (battery_level > BT_GATT_BAS_VAL_MAX) {
		LOG_ERR("Unexpected notification value.");
		if (bas_c->notify_cb) {
			bas_c->notify_cb(bas_c, BT_GATT_BAS_VAL_INVALID);
		}
		return BT_GATT_ITER_STOP;
	}
	bas_c->battery_level = battery_level;
	if (bas_c->notify_cb) {
		bas_c->notify_cb(bas_c, battery_level);
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
static u8_t read_process(struct bt_conn *conn, u8_t err,
			     struct bt_gatt_read_params *params,
			     const void *data, u16_t length)
{
	struct bt_gatt_bas_c *bas_c;
	u8_t battery_level = BT_GATT_BAS_VAL_INVALID;
	const u8_t *bdata = data;

	bas_c = CONTAINER_OF(params, struct bt_gatt_bas_c, read_params);

	if (!bas_c->read_cb) {
		LOG_ERR("No read callback present");
	} else  if (err) {
		LOG_ERR("Read value error: %d", err);
		bas_c->read_cb(bas_c,  battery_level, err);
	} else if (!data || length != 1) {
		bas_c->read_cb(bas_c,  battery_level, -EMSGSIZE);
	} else {
		battery_level = bdata[0];
		if (battery_level > BT_GATT_BAS_VAL_MAX) {
			LOG_ERR("Unexpected read value.");
			bas_c->read_cb(bas_c, BT_GATT_BAS_VAL_INVALID, err);
		} else {
			bas_c->battery_level = battery_level;
			bas_c->read_cb(bas_c, battery_level, err);
		}
	}

	bas_c->read_cb = NULL;
	return BT_GATT_ITER_STOP;
}


void bt_gatt_bas_c_init(struct bt_gatt_bas_c *bas_c)
{
	memset(bas_c, 0, sizeof(*bas_c));
	bas_c->battery_level = BT_GATT_BAS_VAL_INVALID;
}


int bt_gatt_bas_c_handles_assign(struct bt_gatt_dm *dm,
				 struct bt_gatt_bas_c *bas_c)
{
	const struct bt_gatt_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_attr *gatt_chrc;
	const struct bt_gatt_attr *gatt_desc;
	const struct bt_gatt_chrc *chrc_val;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_BAS)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from battery service.");

	bt_gatt_bas_c_init(bas_c);

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
	bas_c->properties = chrc_val->properties;
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_BAS_BATTERY_LEVEL);
	if (!gatt_desc) {
		LOG_ERR("No battery level characteristic value found.");
		return -EINVAL;
	}
	bas_c->val_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_ERR("No battery CCC descriptor found.");
		return -EINVAL;
	}
	bas_c->ccc_handle = gatt_desc->handle;

	/* Finally - save connection object */
	bas_c->conn = bt_gatt_dm_conn_get(dm);
	return 0;
}


int bt_gatt_bas_c_subscribe(struct bt_gatt_bas_c *bas_c,
			    bt_gatt_bas_c_notify_cb func)
{
	int err;

	if (!bas_c || !func) {
		return -EINVAL;
	}
	if (!bas_c->conn) {
		return -EINVAL;
	}
	if (!(bas_c->properties & BT_GATT_CHRC_NOTIFY)) {
		return -ENOTSUP;
	}
	if (bas_c->notify_cb) {
		return -EALREADY;
	}

	bas_c->notify_cb = func;

	bas_c->notify_params.notify = notify_process;
	bas_c->notify_params.value = BT_GATT_CCC_NOTIFY;
	bas_c->notify_params.value_handle = bas_c->val_handle;
	bas_c->notify_params.ccc_handle = bas_c->ccc_handle;
	bas_c->notify_params.flags = BT_GATT_SUBSCRIBE_FLAG_VOLATILE;

	LOG_DBG("Subscribe: val: %u, ccc: %u",
		bas_c->notify_params.value_handle,
		bas_c->notify_params.ccc_handle);
	err = bt_gatt_subscribe(bas_c->conn, &bas_c->notify_params);
	if (err) {
		LOG_ERR("Report notification subscribe error: %d.", err);
		bas_c->notify_cb = NULL;
		return err;
	}
	LOG_DBG("Report subscribed.");
	return err;
}


int bt_gatt_bas_c_unsubscribe(struct bt_gatt_bas_c *bas_c)
{
	int err;

	if (!bas_c) {
		return -EINVAL;
	}

	if (!bas_c->notify_params.notify) {
		return -EFAULT;
	}
	err = bt_gatt_unsubscribe(bas_c->conn, &bas_c->notify_params);
	bas_c->notify_params.notify = NULL;
	return err;
}


struct bt_conn *bt_gatt_bas_c_conn(const struct bt_gatt_bas_c *bas_c)
{
	return bas_c->conn;
}


int bt_gatt_bas_c_read(struct bt_gatt_bas_c *bas_c, bt_gatt_bas_c_read_cb func)
{
	int err;

	if (!bas_c || !func) {
		return -EINVAL;
	}
	if (!bas_c->conn) {
		return -EINVAL;
	}
	if (bas_c->read_cb) {
		return -EBUSY;
	}
	bas_c->read_cb = func;
	bas_c->read_params.func = read_process;
	bas_c->read_params.handle_count  = 1;
	bas_c->read_params.single.handle = bas_c->val_handle;
	bas_c->read_params.single.offset = 0;

	err = bt_gatt_read(bas_c->conn, &bas_c->read_params);
	if (err) {
		bas_c->read_cb = NULL;
		return err;
	}
	return 0;
}


int bt_gatt_bas_c_get(struct bt_gatt_bas_c *bas_c)
{
	if (!bas_c) {
		return -EINVAL;
	}
	return bas_c->battery_level;
}
