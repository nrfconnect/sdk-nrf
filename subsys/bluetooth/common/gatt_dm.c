/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/common/gatt_dm.h>

LOG_MODULE_REGISTER(bt_gatt_dm, CONFIG_BT_GATT_DM_LOG_LEVEL);

/* Available sizes: 128, 512, 2048... */
#define CHUNK_SIZE (128 - sizeof(struct k_mem_block_id))

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_attr attrs[CONFIG_BT_GATT_DM_MAX_ATTRS];
static size_t cur_attr_id;

static u8_t *user_data_chunks[CONFIG_BT_GATT_DM_MAX_MEM_CHUNKS];
static size_t cur_chunk_id;
static size_t cur_chunk_data_len;

enum {
	STATE_ATTRS_LOCKED,
	STATE_ATTRS_RELEASE_PENDING,
	STATE_NUM
};
static ATOMIC_DEFINE(state_flags, STATE_NUM);

static const struct bt_gatt_dm_cb *callback;

static void *user_data_store(const void *user_data, size_t len)
{
	u8_t *user_data_loc;

	__ASSERT_NO_MSG(len <= CHUNK_SIZE);

	if (!user_data_chunks[cur_chunk_id]) {
		user_data_chunks[cur_chunk_id] = k_malloc(CHUNK_SIZE);
		cur_chunk_data_len = 0;

		if (!user_data_chunks[cur_chunk_id]) {
			return NULL;
		}
	}

	if (cur_chunk_data_len + len > CHUNK_SIZE) {
		cur_chunk_id++;
		if ((user_data_chunks[cur_chunk_id]) ||
		    (cur_chunk_id >= ARRAY_SIZE(user_data_chunks))) {
			LOG_ERR("Not enough data chunks");

			return NULL;
		}

		return user_data_store(user_data, len);
	}

	user_data_loc = &user_data_chunks[cur_chunk_id][cur_chunk_data_len];
	memcpy(user_data_loc, user_data, len);
	cur_chunk_data_len += len;

	return user_data_loc;
}

static void svc_attr_memory_release(void)
{
	LOG_DBG("Attr memory release");
	/* Clear attributes */
	memset(attrs, 0, sizeof(attrs));
	cur_attr_id = 0;
	/* Release dynamic memory data chunks */
	for (size_t i = 0; i <= cur_chunk_id; i++) {
		k_free(user_data_chunks[i]);
		user_data_chunks[i] = NULL;
	}
	cur_chunk_id = 0;
	cur_chunk_data_len = 0;
}

static struct bt_gatt_attr *attr_store(const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *cur_attr;

	LOG_DBG("Attr store, pos: %u, handle: %u", cur_attr_id, attr->handle);
	if (cur_attr_id >= ARRAY_SIZE(attrs)) {
		LOG_ERR("No space for new attribute.");
		return NULL;
	}
	cur_attr = &attrs[cur_attr_id++];
	memcpy(cur_attr, attr, sizeof(*attr));

	return cur_attr;
}

static struct bt_gatt_attr *attr_find_by_handle(u16_t handle)
{
	size_t lower = 0;
	size_t upper = cur_attr_id;

	while (upper >= lower) {
		size_t m = (lower + upper) / 2;
		struct bt_gatt_attr *cur_attr = &attrs[m];

		if (cur_attr->handle < handle) {
			lower = m + 1;
		} else if (cur_attr->handle > handle) {
			upper = m - 1;
		} else {
			return cur_attr;
		}
	}

	/* handle not found */
	return NULL;
}

static struct bt_uuid *uuid_16_store(const struct bt_uuid *uuid)
{
	struct bt_uuid_16 *uuid_16 = BT_UUID_16(uuid);

	__ASSERT_NO_MSG(uuid->type == BT_UUID_TYPE_16);

	uuid_16 = user_data_store(uuid_16, sizeof(*uuid_16));
	if (!uuid_16) {
		LOG_ERR("Could not store ATT UUID.");

		return NULL;
	}

	return &uuid_16->uuid;
}

static struct bt_uuid *uuid_128_store(const struct bt_uuid *uuid)
{
	struct bt_uuid_128 *uuid_128 = BT_UUID_128(uuid);

	__ASSERT_NO_MSG(uuid->type == BT_UUID_TYPE_128);

	uuid_128 = user_data_store(uuid_128, sizeof(*uuid_128));

	if (!uuid_128) {
		LOG_ERR("Could not store ATT UUID.");

		return NULL;
	}

	return &uuid_128->uuid;
}

static struct bt_uuid *uuid_store(const struct bt_uuid *uuid)
{
	if (!uuid) {
		LOG_ERR("Uninitialized UUID.");

		return NULL;
	}

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		return uuid_16_store(uuid);
	case BT_UUID_TYPE_128:
		return uuid_128_store(uuid);
	default:
		LOG_ERR("Unsupported UUID type.");
		return NULL;
	}
}

static void discovery_complete(struct bt_conn *conn)
{
	LOG_DBG("Discovery complete.");
	atomic_set_bit(state_flags, STATE_ATTRS_RELEASE_PENDING);
	if (callback->completed) {
		callback->completed(conn, attrs, cur_attr_id);
	}
}

static void discovery_complete_not_found(struct bt_conn *conn)
{
	LOG_DBG("Discover complete. No service found.");

	svc_attr_memory_release();
	atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);

	if (callback->service_not_found) {
		callback->service_not_found(conn);
	}
}

static void discovery_complete_error(struct bt_conn *conn, int err)
{
	svc_attr_memory_release();
	atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);
	if (callback->error_found) {
		callback->error_found(conn, err);
	}
}

static u8_t discovery_process_service(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		discovery_complete_not_found(conn);
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_service_val *service_val = attr->user_data;
	struct bt_gatt_attr *cur_attr = attr_store(attr);

	if (!cur_attr) {
		LOG_ERR("Not enough memory for service attribute.");
		discovery_complete_error(conn, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}
	LOG_DBG("Service detected, handles range: <%u, %u>",
		attr->handle + 1,
		service_val->end_handle);

	cur_attr->uuid = uuid_store(attr->uuid);
	service_val->uuid = uuid_store(service_val->uuid);
	cur_attr->user_data = user_data_store(service_val,
			sizeof(*service_val));
	if (!cur_attr->uuid || !cur_attr->user_data ||
	    !service_val->uuid) {
		LOG_ERR("Not enough memory for service attribute data.");
		discovery_complete_error(conn, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	discover_params.uuid = NULL;
	discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	discover_params.start_handle = attr->handle + 1;
	discover_params.end_handle = service_val->end_handle;
	LOG_DBG("Starting descriptors discovery");
	err = bt_gatt_discover(conn, &discover_params);

	if (err) {
		LOG_ERR("Descriptor discover failed, error: %d.", err);
		discovery_complete_error(conn, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static u8_t discovery_process_descriptor(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	if (!attr) {
		if (cur_attr_id > 1) {
			LOG_DBG("Starting characteristic discovery");
			discover_params.start_handle = attrs[0].handle + 1;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			int err = bt_gatt_discover(conn, &discover_params);

			if (err) {
				LOG_ERR("Characteristic discover failed,"
					" error: %d.",
					err);
				discovery_complete_error(conn, err);
			}
		} else {
			discovery_complete(conn);
		}
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_attr *cur_attr = attr_store(attr);

	if (!cur_attr) {
		LOG_ERR("Not enough memory for next attribute descriptor"
			" at handle %u.",
			attr->handle);
		discovery_complete_error(conn, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}
	cur_attr->uuid = uuid_store(attr->uuid);
	if (!cur_attr->uuid) {
		LOG_ERR("Not enough memory for UUID at handle %u.",
			attr->handle);
		discovery_complete_error(conn, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static u8_t discovery_process_characteristic(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     struct bt_gatt_discover_params *params)
{
	if (!attr) {
		discovery_complete(conn);
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_attr *cur_attr = attr_find_by_handle(attr->handle);
	struct bt_gatt_chrc *gatt_chrc;

	if (!cur_attr) {
		/* Note: We should never be here is the server is working properly */
		discovery_complete_error(conn, -ESRCH);
		return BT_GATT_ITER_STOP;
	}

	gatt_chrc = attr->user_data;
	gatt_chrc->uuid = uuid_store(gatt_chrc->uuid);
	cur_attr->user_data = user_data_store(gatt_chrc,
			sizeof(*gatt_chrc));
	if (!cur_attr->user_data || !gatt_chrc->uuid) {
		discovery_complete_error(conn, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static u8_t discovery_callback(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	if (!attr) {
		LOG_DBG("NULL attribute");
	} else {
		LOG_DBG("Attr: handle %u", attr->handle);
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
	case BT_GATT_DISCOVER_SECONDARY:
		return discovery_process_service(conn, attr, params);
	case BT_GATT_DISCOVER_DESCRIPTOR:
		return discovery_process_descriptor(conn, attr, params);
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		return discovery_process_characteristic(conn, attr, params);
	default:
		/* Error here means some error in this code - should not be possible */
		__ASSERT(false, "Unknown param type.");
		break;
	}

	return BT_GATT_ITER_STOP;
}

int bt_gatt_dm_start(struct bt_conn *conn,
		     const struct bt_uuid *svc_uuid,
		     const struct bt_gatt_dm_cb *cb)
{
	int err;

	if ((svc_uuid->type != BT_UUID_TYPE_16) &&
	    (svc_uuid->type != BT_UUID_TYPE_128)) {
		return -EINVAL;
	}

	if (!cb) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(state_flags, STATE_ATTRS_LOCKED)) {
		return -EALREADY;
	}

	callback = cb;
	cur_attr_id = 0;
	cur_chunk_data_len = 0;
	cur_chunk_id = 0;

	discover_params.uuid = uuid_store(svc_uuid);
	discover_params.func = discovery_callback;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		LOG_ERR("Discover failed, error: %d.", err);
		atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);
	}

	return err;
}

int bt_gatt_dm_data_release(void)
{
	if (!atomic_test_and_clear_bit(state_flags,
				       STATE_ATTRS_RELEASE_PENDING)) {
		return -EALREADY;
	}

	svc_attr_memory_release();
	atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);

	return 0;
}

#if CONFIG_BT_GATT_DM_DATA_PRINT

#define UUID_STR_LEN 37

static void svc_attr_data_print(const struct bt_gatt_service_val *gatt_service)
{
	char str[UUID_STR_LEN];

	bt_uuid_to_str(gatt_service->uuid, str, sizeof(str));
	printk("\tService: 0x%s\tEnd Handle: 0x%04X\n",
			str, gatt_service->end_handle);
}

static void chrc_attr_data_print(const struct bt_gatt_chrc *gatt_chrc)
{
	char str[UUID_STR_LEN];

	bt_uuid_to_str(gatt_chrc->uuid, str, sizeof(str));
	printk("\tCharacteristic: 0x%s\tProperties: 0x%04X\n",
			str, gatt_chrc->properties);
}

static void attr_print(const struct bt_gatt_attr *attr)
{
	char str[UUID_STR_LEN];

	bt_uuid_to_str(attr->uuid, str, sizeof(str));
	printk("ATT[%d]: \tUUID: 0x%s\tHandle: 0x%04X\tValue:\n",
	       (attr - attrs), str, attr->handle);

	if ((bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) == 0) ||
	    (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY) == 0)) {
		svc_attr_data_print(attr->user_data);
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
		chrc_attr_data_print(attr->user_data);
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
		printk("\tCCCD\n");
	}
}

void bt_gatt_dm_data_print(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(attrs); i++) {
		if (attrs[i].uuid != NULL) {
			attr_print(&attrs[i]);
		}
	}
}

#endif /* CONFIG_BT_GATT_DM_DATA_PRINT */
