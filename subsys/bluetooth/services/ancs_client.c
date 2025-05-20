/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Disclaimer: This client implementation of the Apple Notification Center
 * Service can and will be changed at any time by Nordic Semiconductor ASA.
 * Server implementations such as the ones found in iOS can be changed at any
 * time by Apple and may cause this client implementation to stop working.
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/ancs_client.h>
#include "ancs_client_internal.h"
#include "ancs_attr_parser.h"
#include "ancs_app_attr_get.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ancs_c, CONFIG_BT_ANCS_CLIENT_LOG_LEVEL);

/**< Index of the Event ID field when parsing notifications. */
#define BT_ANCS_NOTIF_EVT_ID_INDEX 0
/**< Index of the Flags field when parsing notifications. */
#define BT_ANCS_NOTIF_FLAGS_INDEX 1
/**< Index of the Category ID field when parsing notifications. */
#define BT_ANCS_NOTIF_CATEGORY_ID_INDEX 2
/**< Index of the Category Count field when parsing notifications. */
#define BT_ANCS_NOTIF_CATEGORY_CNT_INDEX 3
/**< Index of the Notification UID field when parsing notifications. */
#define BT_ANCS_NOTIF_NOTIF_UID 4

/**@brief Function for checking whether the data in an iOS notification is out of bounds.
 *
 * @param[in] notif  An iOS notification.
 *
 * @retval 0       If the notification is within bounds.
 * @retval -EINVAL If the notification is out of bounds.
 */
static int bt_ancs_verify_notification_format(const struct bt_ancs_evt_notif *notif)
{
	if ((notif->evt_id >= BT_ANCS_EVT_ID_COUNT) ||
	    (notif->category_id >= BT_ANCS_CATEGORY_ID_COUNT)) {
		return -EINVAL;
	}
	return 0;
}

/**@brief Function for receiving and validating notifications received from
 *        the Notification Provider.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to the data that was received from the Notification Provider.
 * @param[in] len      Length of the data that was received from the Notification Provider.
 */
static void parse_notif(struct bt_ancs_client *ancs_c,
			const uint8_t *data_src, const uint16_t len)
{
	int err;
	struct bt_ancs_evt_notif notif = {0};

	if (len != BT_ANCS_NOTIF_DATA_LENGTH) {
		bt_ancs_do_ns_notif_cb(ancs_c, -EINVAL, &notif);
		return;
	}

	notif.evt_id = (enum bt_ancs_evt_id_values)
		data_src[BT_ANCS_NOTIF_EVT_ID_INDEX];

	notif.evt_flags.silent =
		(data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
		 BT_ANCS_EVENT_FLAG_SILENT) &
		0x01;

	notif.evt_flags.important =
		(data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
		 BT_ANCS_EVENT_FLAG_IMPORTANT) &
		0x01;

	notif.evt_flags.pre_existing =
		(data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
		 BT_ANCS_EVENT_FLAG_PREEXISTING) &
		0x01;

	notif.evt_flags.positive_action =
		(data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
		 BT_ANCS_EVENT_FLAG_POSITIVE_ACTION) &
		0x01;

	notif.evt_flags.negative_action =
		(data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
		 BT_ANCS_EVENT_FLAG_NEGATIVE_ACTION) &
		0x01;

	notif.category_id = (enum bt_ancs_category_id_val)
		data_src[BT_ANCS_NOTIF_CATEGORY_ID_INDEX];

	notif.category_count =
		data_src[BT_ANCS_NOTIF_CATEGORY_CNT_INDEX];
	notif.notif_uid =
		sys_get_le32(&data_src[BT_ANCS_NOTIF_NOTIF_UID]);

	err = bt_ancs_verify_notification_format(&notif);

	bt_ancs_do_ns_notif_cb(ancs_c, err, &notif);
}

static uint8_t on_received_ns(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	struct bt_ancs_client *ancs_c;

	/* Retrieve ANCS client module context. */
	ancs_c = CONTAINER_OF(params, struct bt_ancs_client, ns_notif_params);

	parse_notif(ancs_c, data, length);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_received_ds(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	struct bt_ancs_client *ancs_c;

	/* Retrieve ANCS client module context. */
	ancs_c = CONTAINER_OF(params, struct bt_ancs_client, ds_notif_params);

	bt_ancs_parse_get_attrs_response(ancs_c, data, length);

	return BT_GATT_ITER_CONTINUE;
}

int bt_ancs_client_init(struct bt_ancs_client *ancs_c)
{
	if (!ancs_c) {
		return -EINVAL;
	}

	memset(ancs_c, 0, sizeof(struct bt_ancs_client));

	return 0;
}

static void ancs_reinit(struct bt_ancs_client *ancs_c)
{
	ancs_c->conn = NULL;
	ancs_c->handle_cp = 0;
	ancs_c->handle_ns = 0;
	ancs_c->handle_ns_ccc = 0;
	ancs_c->handle_ds = 0;
	ancs_c->handle_ds_ccc = 0;
	ancs_c->state = ATOMIC_INIT(0);
}

int bt_ancs_handles_assign(struct bt_gatt_dm *dm,
			   struct bt_ancs_client *ancs_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_ANCS)) {
		return -ENOTSUP;
	}
	LOG_DBG("ANCS found");

	ancs_reinit(ancs_c);

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_ANCS_CONTROL_POINT);
	if (!gatt_chrc) {
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_ANCS_CONTROL_POINT);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ancs_c->handle_cp = gatt_desc->handle;
	LOG_DBG("Control Point characteristic found.");

	gatt_chrc =
		bt_gatt_dm_char_by_uuid(dm, BT_UUID_ANCS_NOTIFICATION_SOURCE);
	if (!gatt_chrc) {
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_ANCS_NOTIFICATION_SOURCE);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ancs_c->handle_ns = gatt_desc->handle;
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ancs_c->handle_ns_ccc = gatt_desc->handle;
	LOG_DBG("Notification Source characteristic found.");

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_ANCS_DATA_SOURCE);
	if (!gatt_chrc) {
		return -EINVAL;
	}
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_ANCS_DATA_SOURCE);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ancs_c->handle_ds = gatt_desc->handle;
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ancs_c->handle_ds_ccc = gatt_desc->handle;
	LOG_DBG("Data Source characteristic found.");

	/* Finally - save connection object */
	ancs_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

int bt_ancs_subscribe_notification_source(struct bt_ancs_client *ancs_c,
					  bt_ancs_ns_notif_cb func)
{
	int err;

	if (!ancs_c || !func) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	ancs_c->ns_notif_params.notify = on_received_ns;
	ancs_c->ns_notif_params.value = BT_GATT_CCC_NOTIFY;
	ancs_c->ns_notif_params.value_handle = ancs_c->handle_ns;
	ancs_c->ns_notif_params.ccc_handle = ancs_c->handle_ns_ccc;
	atomic_set_bit(ancs_c->ns_notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	ancs_c->ns_notif_cb = func;

	err = bt_gatt_subscribe(ancs_c->conn, &ancs_c->ns_notif_params);
	if (err) {
		atomic_clear_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED);
		LOG_ERR("Subscribe Notification Source failed (err %d)", err);
	} else {
		LOG_DBG("Notification Source subscribed");
	}

	return err;
}

int bt_ancs_subscribe_data_source(struct bt_ancs_client *ancs_c,
				  bt_ancs_ds_notif_cb func)
{
	int err;

	if (!ancs_c || !func) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	ancs_c->ds_notif_params.notify = on_received_ds;
	ancs_c->ds_notif_params.value = BT_GATT_CCC_NOTIFY;
	ancs_c->ds_notif_params.value_handle = ancs_c->handle_ds;
	ancs_c->ds_notif_params.ccc_handle = ancs_c->handle_ds_ccc;
	atomic_set_bit(ancs_c->ds_notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	ancs_c->ds_notif_cb = func;

	err = bt_gatt_subscribe(ancs_c->conn, &ancs_c->ds_notif_params);
	if (err) {
		atomic_clear_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED);
		LOG_ERR("Subscribe Data Source failed (err %d)", err);
	} else {
		LOG_DBG("Data Source subscribed");
	}

	return err;
}

int bt_ancs_unsubscribe_notification_source(struct bt_ancs_client *ancs_c)
{
	int err;

	if (!ancs_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(ancs_c->conn, &ancs_c->ns_notif_params);
	if (err) {
		LOG_ERR("Unsubscribe Notification Source failed (err %d)", err);
	} else {
		atomic_clear_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED);
		LOG_DBG("Notification Source unsubscribed");
	}

	return err;
}

int bt_ancs_unsubscribe_data_source(struct bt_ancs_client *ancs_c)
{
	int err;

	if (!ancs_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(ancs_c->conn, &ancs_c->ds_notif_params);
	if (err) {
		LOG_ERR("Unsubscribe Data Source failed (err %d)", err);
	} else {
		atomic_clear_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED);
		LOG_DBG("Data Source unsubscribed");
	}

	return err;
}

static uint16_t encode_notif_action(uint8_t *encoded_data, uint32_t uid,
				    enum bt_ancs_action_id_values action_id)
{
	uint8_t index = 0;

	encoded_data[index++] = BT_ANCS_COMMAND_ID_PERFORM_NOTIF_ACTION;
	sys_put_le32(uid, &encoded_data[index]);
	index += sizeof(uint32_t);
	encoded_data[index++] = (uint8_t)action_id;

	return index;
}

static void bt_ancs_cp_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ancs_client *ancs_c;
	bt_ancs_write_cb write_cb;

	/* Retrieve ANCS client module context. */
	ancs_c = CONTAINER_OF(params, struct bt_ancs_client, cp_write_params);

	write_cb = ancs_c->cp_write_cb;
	atomic_clear_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING);
	if (write_cb) {
		write_cb(ancs_c, err);
	}
}

int bt_ancs_cp_write(struct bt_ancs_client *ancs_c, uint16_t len,
		     bt_ancs_write_cb func)
{
	int err;
	struct bt_gatt_write_params *write_params = &ancs_c->cp_write_params;

	write_params->func = bt_ancs_cp_write_callback;
	write_params->handle = ancs_c->handle_cp;
	write_params->offset = 0;
	write_params->data = ancs_c->cp_data;
	write_params->length = len;

	ancs_c->cp_write_cb = func;

	err = bt_gatt_write(ancs_c->conn, write_params);
	if (err) {
		atomic_clear_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING);
	}

	return err;
}

int bt_ancs_notification_action(struct bt_ancs_client *ancs_c, uint32_t uuid,
				enum bt_ancs_action_id_values action_id,
				bt_ancs_write_cb func)
{
	if (atomic_test_and_set_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING)) {
		return -EBUSY;
	}

	uint8_t *data = ancs_c->cp_data;
	uint16_t len = encode_notif_action(data, uuid, action_id);

	return bt_ancs_cp_write(ancs_c, len, func);
}

static int bt_ancs_get_notif_attrs(struct bt_ancs_client *ancs_c,
				   const uint32_t uid, bt_ancs_write_cb func)
{
	if (atomic_test_and_set_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING)) {
		return -EBUSY;
	}

	uint32_t index = 0;
	uint8_t *data = ancs_c->cp_data;

	ancs_c->number_of_requested_attr = 0;

	/* Encode Command ID. */
	*(data + index++) = BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES;

	/* Encode Notification UID. */
	sys_put_le32(uid, data + index);
	index += sizeof(uint32_t);

	/* Encode Attribute ID. */
	for (uint32_t attr = 0; attr < BT_ANCS_NOTIF_ATTR_COUNT; attr++) {
		if (ancs_c->ancs_notif_attr_list[attr].get) {
			*(data + index++) = (uint8_t)attr;

			if ((attr == BT_ANCS_NOTIF_ATTR_ID_TITLE) ||
			    (attr == BT_ANCS_NOTIF_ATTR_ID_SUBTITLE) ||
			    (attr == BT_ANCS_NOTIF_ATTR_ID_MESSAGE)) {
				/* Encode Length field. Only applicable for
				 * Title, Subtitle, and Message.
				 */
				sys_put_le16(ancs_c->ancs_notif_attr_list[attr]
						     .attr_len,
					     data + index);
				index += sizeof(uint16_t);
			}

			ancs_c->number_of_requested_attr++;
		}
	}

	ancs_c->parse_info.expected_number_of_attrs =
		ancs_c->number_of_requested_attr;

	return bt_ancs_cp_write(ancs_c, index, func);
}

int bt_ancs_request_attrs(struct bt_ancs_client *ancs_c,
			  const struct bt_ancs_evt_notif *notif,
			  bt_ancs_write_cb func)
{
	int err;

	err = bt_ancs_verify_notification_format(notif);
	if (err) {
		return err;
	}

	ancs_c->parse_info.parse_state = BT_ANCS_PARSE_STATE_COMMAND_ID;

	return bt_ancs_get_notif_attrs(ancs_c, notif->notif_uid, func);
}

int bt_ancs_register_attr(struct bt_ancs_client *ancs_c,
			  const enum bt_ancs_notif_attr_id_val id,
			  uint8_t *data, const uint16_t len)
{
	if (!ancs_c || !data) {
		return -EINVAL;
	}

	if (!len || len > BT_ANCS_ATTR_DATA_MAX) {
		return -EINVAL;
	}

	if ((size_t)id >= BT_ANCS_NOTIF_ATTR_COUNT) {
		return -EINVAL;
	}

	ancs_c->ancs_notif_attr_list[id].get = true;
	ancs_c->ancs_notif_attr_list[id].attr_len = len;
	ancs_c->ancs_notif_attr_list[id].attr_data = data;

	return 0;
}

int bt_ancs_register_app_attr(struct bt_ancs_client *ancs_c,
			      const enum bt_ancs_app_attr_id_val id,
			      uint8_t *data, const uint16_t len)
{
	if (!ancs_c || !data) {
		return -EINVAL;
	}

	if (!len || len > BT_ANCS_ATTR_DATA_MAX) {
		return -EINVAL;
	}

	if ((size_t)id >= BT_ANCS_APP_ATTR_COUNT) {
		return -EINVAL;
	}

	ancs_c->ancs_app_attr_list[id].get = true;
	ancs_c->ancs_app_attr_list[id].attr_len = len;
	ancs_c->ancs_app_attr_list[id].attr_data = data;

	return 0;
}

int bt_ancs_request_app_attr(struct bt_ancs_client *ancs_c,
			     const uint8_t *app_id, uint32_t len,
			     bt_ancs_write_cb func)
{
	return bt_ancs_app_attr_request(ancs_c, app_id, len, func);
}
