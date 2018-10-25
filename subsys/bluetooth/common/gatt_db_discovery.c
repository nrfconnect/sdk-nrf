/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <bluetooth/common/gatt_db_discovery.h>

#define LOG_MODULE_NAME gatt_db_discovery
#define LOG_LEVEL CONFIG_NRF_BT_GATT_DB_DISCOVERY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER();

/* Available sizes: 128, 512, 2048... */
#define CHUNK_SIZE (128 - sizeof(struct k_mem_block_id))

#define ATTRS_LEN_GET(svc_attr) \
	(((struct bt_gatt_service_val *) svc_attr.user_data)->end_handle \
	 - svc_attr.handle + 1)

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_attr attrs[CONFIG_NRF_BT_GATT_DB_DISCOVERY_MAX_ATTRS];

static u8_t *user_data_chunks[CONFIG_NRF_BT_GATT_DB_DISCOVERY_MAX_MEM_CHUNKS];
static u8_t cur_chunk_id;
static u8_t cur_chunk_data_len;

enum {
	STATE_ATTRS_LOCKED,
	STATE_ATTRS_RELEASE_PENDING,
	STATE_NUM
};
static ATOMIC_DEFINE(state_flags, STATE_NUM);

static struct gatt_db_discovery_cb *callback;

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
	for (size_t i = 0; i < ARRAY_SIZE(user_data_chunks); i++) {
		if (user_data_chunks[i]) {
			k_free(user_data_chunks[i]);
			user_data_chunks[i] = NULL;
		}
	}

	cur_chunk_data_len = 0;
	cur_chunk_id = 0;

	memset(attrs, 0, sizeof(attrs));
}

static struct bt_gatt_attr *attr_store(const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *cur_attr;
	u16_t i;

	if (!attrs[0].uuid) {
		cur_attr = &attrs[0];
	} else {
		i = attr->handle - attrs[0].handle;
		if (i >= ARRAY_SIZE(attrs)) {
			LOG_ERR("New attribute index is out of the "
				    "declared range.");

			return NULL;
		}

		cur_attr = &attrs[i];
	}

	memcpy(cur_attr, attr, sizeof(*attr));

	return cur_attr;
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

static bool is_desc_dicovery_reqd(u16_t *start_handle, u16_t *end_handle)
{
	bool in_chrc_range = false;
	u16_t start_handle_int = *start_handle;
	struct bt_gatt_service_val *gatt_service = attrs[0].user_data;

	for (size_t i = *start_handle - attrs[0].handle; i < ARRAY_SIZE(attrs);
	     i++) {
		if (!attrs[i].uuid && !in_chrc_range) {
			in_chrc_range = true;
			*start_handle = attrs[i - 1].handle + 1;
		}

		if (attrs[i].uuid && in_chrc_range) {
			in_chrc_range = false;
			*end_handle = attrs[i].handle - 1;
			break;
		}
	}

	/* The last group of characteristic descriptors. */
	if (in_chrc_range) {
		*end_handle = gatt_service->end_handle;
	}

	/* No place for more descriptors. */
	if (start_handle_int == *start_handle) {
		return false;
	}

	return true;
}

static int descriptor_discovery_start(struct bt_conn *conn)
{
	int err = 0;
	bool is_disc_reqd;

	discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

	is_disc_reqd = is_desc_dicovery_reqd(&discover_params.start_handle,
					     &discover_params.end_handle);
	if (is_disc_reqd) {
		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			LOG_ERR("Discover failed, error: %d.", err);

			return err;
		}
	} else {
		LOG_DBG("Discovery complete.");
		atomic_set_bit(state_flags, STATE_ATTRS_RELEASE_PENDING);
		if (callback->completed) {
			callback->completed(conn, attrs,
					    ATTRS_LEN_GET(attrs[0]));
		}
	}

	return err;
}

static u8_t discovery_callback(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	int err = 0;
	struct bt_gatt_attr *cur_attr;
	struct bt_gatt_service_val *gatt_service;
	struct bt_gatt_chrc *gatt_chrc;

	if (!attr) {
		if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
			discover_params.start_handle = attrs[0].handle;

			err = descriptor_discovery_start(conn);
			if (err) {
				goto error;
			}
		} else if (params->type == BT_GATT_DISCOVER_DESCRIPTOR) {
			err = descriptor_discovery_start(conn);
			if (err) {
				goto error;
			}
		} else {
			LOG_DBG("Discover complete. No service found.");

			svc_attr_memory_release();
			atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);

			if (callback->service_not_found) {
				callback->service_not_found(conn);
			}

			return BT_GATT_ITER_STOP;
		}

		return BT_GATT_ITER_STOP;
	}

	cur_attr = attr_store(attr);
	if (!cur_attr) {
		LOG_ERR("Not enough memory for next attribute descriptor.");
		goto error;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
	case BT_GATT_DISCOVER_SECONDARY:
		gatt_service = attr->user_data;

		cur_attr->uuid = uuid_store(attr->uuid);
		gatt_service->uuid = uuid_store(gatt_service->uuid);
		cur_attr->user_data = user_data_store(gatt_service,
				sizeof(*gatt_service));
		if (!cur_attr->uuid || !cur_attr->user_data ||
		    !gatt_service->uuid) {
			err = -ENOMEM;
			goto error;
		}

		discover_params.uuid = NULL;
		discover_params.start_handle = attr->handle;
		discover_params.end_handle = gatt_service->end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			LOG_ERR("Discover failed, error: %d.", err);
			goto error;
		}

		return BT_GATT_ITER_STOP;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;

		cur_attr->uuid = uuid_store(attr->uuid);
		gatt_chrc->uuid = uuid_store(gatt_chrc->uuid);
		cur_attr->user_data = user_data_store(gatt_chrc,
				sizeof(*gatt_chrc));
		if (!cur_attr->uuid || !cur_attr->user_data ||
		    !gatt_chrc->uuid) {
			err = -ENOMEM;
			goto error;
		}

		return BT_GATT_ITER_CONTINUE;
	case BT_GATT_DISCOVER_DESCRIPTOR:
		cur_attr->uuid = uuid_store(attr->uuid);
		if (!cur_attr->uuid) {
			err = -ENOMEM;
			goto error;
		}

		return BT_GATT_ITER_CONTINUE;
	default:
		LOG_ERR("Unknown param type.");
		break;
	}

error:
	svc_attr_memory_release();
	atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);
	if (callback->error_found) {
		callback->error_found(conn, err);
	}

	return BT_GATT_ITER_STOP;
}

int gatt_db_discovery_start(struct bt_conn *conn,
			    struct bt_uuid *svc_uuid,
			    struct gatt_db_discovery_cb *cb)
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

int gatt_db_discovery_data_release(void)
{
	if (!atomic_test_and_clear_bit(state_flags,
				       STATE_ATTRS_RELEASE_PENDING)) {
		return -EALREADY;
	}

	svc_attr_memory_release();
	atomic_clear_bit(state_flags, STATE_ATTRS_LOCKED);

	return 0;
}

#if CONFIG_NRF_BT_GATT_DB_DISCOVERY_DATA_PRINT

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

void gatt_db_discovery_data_print(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(attrs); i++) {
		if (attrs[i].uuid != NULL) {
			attr_print(&attrs[i]);
		}
	}
}

#endif /* CONFIG_NRF_BT_GATT_DB_DISCOVERY_DATA_PRINT */
