/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/ams_client.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ams_c, CONFIG_BT_AMS_CLIENT_LOG_LEVEL);

enum {
	AMS_RC_WRITE_PENDING,
	AMS_RC_NOTIF_ENABLED,
	AMS_EU_WRITE_PENDING,
	AMS_EU_NOTIF_ENABLED,
	AMS_EA_WRITE_PENDING,
	AMS_EA_READ_PENDING
};

int bt_ams_client_init(struct bt_ams_client *ams_c)
{
	if (!ams_c) {
		return -EINVAL;
	}

	memset(ams_c, 0, sizeof(struct bt_ams_client));

	return 0;
}

static void ams_reinit(struct bt_ams_client *ams_c)
{
	ams_c->conn = NULL;
	ams_c->remote_command.handle = 0;
	ams_c->remote_command.handle_ccc = 0;
	ams_c->entity_update.handle = 0;
	ams_c->entity_update.handle_ccc = 0;
	ams_c->entity_attribute.handle = 0;
	ams_c->state = ATOMIC_INIT(0);
}

int bt_ams_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_ams_client *ams_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_AMS)) {
		return -ENOTSUP;
	}

	LOG_DBG("AMS found");

	ams_reinit(ams_c);

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AMS_REMOTE_COMMAND);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_AMS_REMOTE_COMMAND);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->remote_command.handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->remote_command.handle_ccc = gatt_desc->handle;

	LOG_DBG("Remote Command characteristic found.");

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AMS_ENTITY_UPDATE);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_AMS_ENTITY_UPDATE);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->entity_update.handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->entity_update.handle_ccc = gatt_desc->handle;

	LOG_DBG("Entity Update characteristic found.");

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AMS_ENTITY_ATTRIBUTE);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_AMS_ENTITY_ATTRIBUTE);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->entity_attribute.handle = gatt_desc->handle;

	LOG_DBG("Entity Attribute characteristic found.");

	/* Finally - save connection object */
	ams_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

static uint8_t bt_ams_rc_notify_callback(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	struct bt_ams_client *ams_c;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, remote_command.notif_params);

	if (ams_c->remote_command.notify_cb) {
		ams_c->remote_command.notify_cb(ams_c, data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_ams_subscribe_remote_command(struct bt_ams_client *ams_c,
				    bt_ams_remote_command_notify_cb func)
{
	int err;

	if (!ams_c || !ams_c->conn || !func) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	ams_c->remote_command.notify_cb = func;

	ams_c->remote_command.notif_params.notify = bt_ams_rc_notify_callback;
	ams_c->remote_command.notif_params.value = BT_GATT_CCC_NOTIFY;
	ams_c->remote_command.notif_params.value_handle = ams_c->remote_command.handle;
	ams_c->remote_command.notif_params.ccc_handle = ams_c->remote_command.handle_ccc;
	atomic_set_bit(ams_c->remote_command.notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(ams_c->conn, &ams_c->remote_command.notif_params);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED);
		LOG_ERR("Subscribe Remote Command failed (err %d)", err);
	} else {
		LOG_DBG("Remote Command subscribed");
	}

	return err;
}

int bt_ams_unsubscribe_remote_command(struct bt_ams_client *ams_c)
{
	int err;

	if (!ams_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(ams_c->conn, &ams_c->remote_command.notif_params);
	if (err) {
		LOG_ERR("Unsubscribe Remote Command failed (err %d)", err);
	} else {
		atomic_clear_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED);
		LOG_DBG("Remote Command unsubscribed");
	}

	return err;
}

static uint8_t bt_ams_eu_notify_callback(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	struct bt_ams_client *ams_c;
	const uint8_t *value = data;
	struct bt_ams_entity_update_notif notif;
	int err;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, entity_update.notif_params);

	err = (length >= BT_AMS_EU_NOTIF_VALUE_IDX) ? 0 : -EINVAL;
	if (!err) {
		notif.ent_attr.entity = value[BT_AMS_EU_NOTIF_ENTITY_IDX];
		switch (notif.ent_attr.entity) {
		case BT_AMS_ENTITY_ID_PLAYER:
			notif.ent_attr.attribute.player =
				value[BT_AMS_EU_NOTIF_ATTRIBUTE_IDX];
			break;
		case BT_AMS_ENTITY_ID_QUEUE:
			notif.ent_attr.attribute.queue =
				value[BT_AMS_EU_NOTIF_ATTRIBUTE_IDX];
			break;
		case BT_AMS_ENTITY_ID_TRACK:
			notif.ent_attr.attribute.track =
				value[BT_AMS_EU_NOTIF_ATTRIBUTE_IDX];
			break;
		default:
			err = -EINVAL;
			break;
		}
		notif.flags = value[BT_AMS_EU_NOTIF_FLAGS_IDX];
		notif.data = &value[BT_AMS_EU_NOTIF_VALUE_IDX];
		notif.len = length - BT_AMS_EU_NOTIF_VALUE_IDX;
	}

	if (ams_c->entity_update.notify_cb) {
		ams_c->entity_update.notify_cb(ams_c, &notif, err);
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_ams_subscribe_entity_update(struct bt_ams_client *ams_c,
				   bt_ams_entity_update_notify_cb func)
{
	int err;

	if (!ams_c || !ams_c->conn || !func) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	ams_c->entity_update.notify_cb = func;

	ams_c->entity_update.notif_params.notify = bt_ams_eu_notify_callback;
	ams_c->entity_update.notif_params.value = BT_GATT_CCC_NOTIFY;
	ams_c->entity_update.notif_params.value_handle = ams_c->entity_update.handle;
	ams_c->entity_update.notif_params.ccc_handle = ams_c->entity_update.handle_ccc;
	atomic_set_bit(ams_c->entity_update.notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(ams_c->conn, &ams_c->entity_update.notif_params);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED);
		LOG_ERR("Subscribe Entity Update failed (err %d)", err);
	} else {
		LOG_DBG("Entity Update subscribed");
	}

	return err;
}

int bt_ams_unsubscribe_entity_update(struct bt_ams_client *ams_c)
{
	int err;

	if (!ams_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(ams_c->conn, &ams_c->entity_update.notif_params);
	if (err) {
		LOG_ERR("Unsubscribe Entity Update failed (err %d)", err);
	} else {
		atomic_clear_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED);
		LOG_DBG("Entity Update unsubscribed");
	}

	return err;
}

static void bt_ams_rc_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ams_client *ams_c;
	bt_ams_write_cb write_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, remote_command.write_params);

	write_callback = ams_c->remote_command.write_cb;
	atomic_clear_bit(&ams_c->state, AMS_RC_WRITE_PENDING);
	if (write_callback) {
		write_callback(ams_c, err);
	}
}

int bt_ams_write_remote_command(struct bt_ams_client *ams_c,
				enum bt_ams_remote_command_id command,
				bt_ams_write_cb func)
{
	int err;

	if (!ams_c || !ams_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_RC_WRITE_PENDING)) {
		return -EBUSY;
	}

	ams_c->remote_command.data = command;

	ams_c->remote_command.write_params.func = bt_ams_rc_write_callback;
	ams_c->remote_command.write_params.handle = ams_c->remote_command.handle;
	ams_c->remote_command.write_params.offset = 0;
	ams_c->remote_command.write_params.data = &ams_c->remote_command.data;
	ams_c->remote_command.write_params.length = 1;

	ams_c->remote_command.write_cb = func;

	err = bt_gatt_write(ams_c->conn, &ams_c->remote_command.write_params);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_RC_WRITE_PENDING);
	}

	return err;
}

static void bt_ams_eu_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ams_client *ams_c;
	bt_ams_write_cb write_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, entity_update.write_params);

	write_callback = ams_c->entity_update.write_cb;
	atomic_clear_bit(&ams_c->state, AMS_EU_WRITE_PENDING);
	if (write_callback) {
		write_callback(ams_c, err);
	}
}

int bt_ams_write_entity_update(struct bt_ams_client *ams_c,
			       const struct bt_ams_entity_attribute_list *ent_attr_list,
			       bt_ams_write_cb func)
{
	int err = 0;
	size_t i;
	uint8_t attr_val;

	if (!ams_c || !ams_c->conn) {
		return -EINVAL;
	}
	if (ent_attr_list->attribute_count > BT_AMS_EU_CMD_ATTRIBUTE_COUNT_MAX) {
		return -ENOMEM;
	};

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EU_WRITE_PENDING)) {
		return -EBUSY;
	}

	ams_c->entity_update.data[BT_AMS_EU_CMD_ENTITY_IDX] = ent_attr_list->entity;
	for (i = 0; i < ent_attr_list->attribute_count; i++) {
		switch (ent_attr_list->entity) {
		case BT_AMS_ENTITY_ID_PLAYER:
			attr_val = ent_attr_list->attribute.player[i];
			break;
		case BT_AMS_ENTITY_ID_QUEUE:
			attr_val = ent_attr_list->attribute.queue[i];
			break;
		case BT_AMS_ENTITY_ID_TRACK:
			attr_val = ent_attr_list->attribute.track[i];
			break;
		default:
			atomic_clear_bit(&ams_c->state, AMS_EU_WRITE_PENDING);
			return -EINVAL;
		}

		ams_c->entity_update.data[i + BT_AMS_EU_CMD_ATTRIBUTE_IDX] = attr_val;
	}

	ams_c->entity_update.write_params.func = bt_ams_eu_write_callback;
	ams_c->entity_update.write_params.handle = ams_c->entity_update.handle;
	ams_c->entity_update.write_params.offset = 0;
	ams_c->entity_update.write_params.data = ams_c->entity_update.data;
	ams_c->entity_update.write_params.length = ent_attr_list->attribute_count + 1;

	ams_c->entity_update.write_cb = func;

	err = bt_gatt_write(ams_c->conn, &ams_c->entity_update.write_params);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EU_WRITE_PENDING);
	}

	return err;
}

static void bt_ams_ea_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ams_client *ams_c;
	bt_ams_write_cb write_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, entity_attribute.write_params);

	write_callback = ams_c->entity_attribute.write_cb;
	atomic_clear_bit(&ams_c->state, AMS_EA_WRITE_PENDING);
	if (write_callback) {
		write_callback(ams_c, err);
	}
}

int bt_ams_write_entity_attribute(struct bt_ams_client *ams_c,
				  const struct bt_ams_entity_attribute *ent_attr,
				  bt_ams_write_cb func)
{
	int err;
	uint8_t attr_val;

	if (!ams_c || !ams_c->conn) {
		return -EINVAL;
	}

	switch (ent_attr->entity) {
	case BT_AMS_ENTITY_ID_PLAYER:
		attr_val = ent_attr->attribute.player;
		break;
	case BT_AMS_ENTITY_ID_QUEUE:
		attr_val = ent_attr->attribute.queue;
		break;
	case BT_AMS_ENTITY_ID_TRACK:
		attr_val = ent_attr->attribute.track;
		break;
	default:
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EA_WRITE_PENDING)) {
		return -EBUSY;
	}

	ams_c->entity_attribute.data[BT_AMS_EA_CMD_ENTITY_IDX] = ent_attr->entity;
	ams_c->entity_attribute.data[BT_AMS_EA_CMD_ATTRIBUTE_IDX] = attr_val;

	ams_c->entity_attribute.write_params.func = bt_ams_ea_write_callback;
	ams_c->entity_attribute.write_params.handle = ams_c->entity_attribute.handle;
	ams_c->entity_attribute.write_params.offset = 0;
	ams_c->entity_attribute.write_params.data = ams_c->entity_attribute.data;
	ams_c->entity_attribute.write_params.length = BT_AMS_EA_CMD_LEN;

	ams_c->entity_attribute.write_cb = func;

	err = bt_gatt_write(ams_c->conn, &ams_c->entity_attribute.write_params);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EA_WRITE_PENDING);
	}

	return err;
}

static uint8_t bt_ams_ea_read_callback(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	struct bt_ams_client *ams_c;
	bt_ams_read_cb read_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, entity_attribute.read_params);

	read_callback = ams_c->entity_attribute.read_cb;
	atomic_clear_bit(&ams_c->state, AMS_EA_READ_PENDING);
	if (read_callback) {
		read_callback(ams_c, err, data, length);
	}

	return BT_GATT_ITER_STOP;
}

int bt_ams_read_entity_attribute(struct bt_ams_client *ams_c, bt_ams_read_cb func)
{
	int err;

	if (!ams_c || !ams_c->conn || !func) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EA_READ_PENDING)) {
		return -EBUSY;
	}

	ams_c->entity_attribute.read_params.func = bt_ams_ea_read_callback;
	ams_c->entity_attribute.read_params.handle_count = 1;
	ams_c->entity_attribute.read_params.single.handle = ams_c->entity_attribute.handle;
	ams_c->entity_attribute.read_params.single.offset = 0;

	ams_c->entity_attribute.read_cb = func;

	err = bt_gatt_read(ams_c->conn, &ams_c->entity_attribute.read_params);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EA_READ_PENDING);
	}

	return err;
}
