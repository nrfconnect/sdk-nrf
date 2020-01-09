/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <inttypes.h>
#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/gatt_dm.h>

LOG_MODULE_REGISTER(bt_gatt_dm, CONFIG_BT_GATT_DM_LOG_LEVEL);

/* Available sizes: 128, 512, 2048... */
#define CHUNK_SIZE (128 - sizeof(struct k_mem_block_id))

#if __CORTEX_M == (0UL)
#define UUID_ALIGN 4U
#else
#define UUID_ALIGN 1U
#endif

/* Flags for parsed attribute array state */
enum {
	STATE_ATTRS_LOCKED,
	STATE_ATTRS_RELEASE_PENDING,
	STATE_NUM
};

/* The instance structure real declaration */
struct bt_gatt_dm {
	/* Connection object */
	struct bt_conn *conn;
	/* The user context */
	void *context;

	/* The discovery parameters used */
	struct bt_gatt_discover_params discover_params;
	/* Currently parsed attributes */
	struct bt_gatt_attr attrs[CONFIG_BT_GATT_DM_MAX_ATTRS];
	/* Currently accessed attribute */
	size_t cur_attr_id;
	/* Flags with the status of the attributes */
	ATOMIC_DEFINE(state_flags, STATE_NUM);

	/* user data chunks */
	struct {
		/* The pointers to the dynamically allocated memory */
		u8_t *chunks[CONFIG_BT_GATT_DM_MAX_MEM_CHUNKS];
		/* Currently processed index */
		size_t cur_id;
		/* The used length of the current chunk */
		size_t cur_len;
	} data_chunk;

	/* The pointer to callback structure */
	const struct bt_gatt_dm_cb *callback;
};

/* Currently only one instance is supported */
static struct bt_gatt_dm bt_gatt_dm_inst;


static void *user_data_store(struct bt_gatt_dm *dm,
			     const void *user_data,
			     size_t len)
{
	u8_t *user_data_loc;
	size_t cur_chunk_id = dm->data_chunk.cur_id;

	__ASSERT_NO_MSG(len <= CHUNK_SIZE);

	if (!(dm->data_chunk.chunks[cur_chunk_id])) {
		dm->data_chunk.chunks[cur_chunk_id] = k_malloc(CHUNK_SIZE);
		dm->data_chunk.cur_len = 0;

		if (!(dm->data_chunk.chunks[cur_chunk_id])) {
			return NULL;
		}
	}

	size_t misalign = dm->data_chunk.cur_len % UUID_ALIGN;
	if (misalign) {
		dm->data_chunk.cur_len += UUID_ALIGN - misalign;
	}

	if (dm->data_chunk.cur_len + len > CHUNK_SIZE) {
		cur_chunk_id++;
		dm->data_chunk.cur_id = cur_chunk_id;
		if ((dm->data_chunk.chunks[cur_chunk_id]) ||
		    (cur_chunk_id >= ARRAY_SIZE(dm->data_chunk.chunks))) {
			LOG_ERR("Not enough data chunks");
			return NULL;
		}

		return user_data_store(dm, user_data, len);
	}

	user_data_loc =
		&dm->data_chunk.chunks[cur_chunk_id][dm->data_chunk.cur_len];
	memcpy(user_data_loc, user_data, len);
	dm->data_chunk.cur_len += len;

	return user_data_loc;
}

static void svc_attr_memory_release(struct bt_gatt_dm *dm)
{
	LOG_DBG("Attr memory release");
	/* Clear attributes */
	memset(dm->attrs, 0, sizeof(dm->attrs));
	dm->cur_attr_id = 0;
	/* Release dynamic memory data chunks */
	for (size_t i = 0; i <= dm->data_chunk.cur_id; i++) {
		k_free(dm->data_chunk.chunks[i]);
		dm->data_chunk.chunks[i] = NULL;
	}
	dm->data_chunk.cur_id = 0;
	dm->data_chunk.cur_len = 0;
}

static struct bt_gatt_attr *attr_store(struct bt_gatt_dm *dm,
				       const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *cur_attr;

	LOG_DBG("Attr store, pos: %zu, handle: %"PRIu16,
		dm->cur_attr_id,
		attr->handle);
	if (dm->cur_attr_id >= ARRAY_SIZE(dm->attrs)) {
		LOG_ERR("No space for new attribute.");
		return NULL;
	}
	cur_attr = &dm->attrs[(dm->cur_attr_id)++];
	memcpy(cur_attr, attr, sizeof(*attr));

	return cur_attr;
}

static const struct bt_gatt_attr *attr_find_by_handle_c(
	const struct bt_gatt_dm *dm,
	u16_t handle)
{
	if (!dm->cur_attr_id) {
		return NULL;
	}

	ssize_t lower = 0;
	ssize_t upper = dm->cur_attr_id - 1;

	while (upper >= lower) {
		size_t m = (lower + upper) / 2;
		const struct bt_gatt_attr *cur_attr = &dm->attrs[m];

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

static struct bt_gatt_attr *attr_find_by_handle(struct bt_gatt_dm *dm,
						u16_t handle)
{
	/* Ugly casting but avoids code duplication for non-const getter */
	return (struct bt_gatt_attr *)attr_find_by_handle_c(dm, handle);
}

static struct bt_uuid *uuid_16_store(struct bt_gatt_dm *dm,
				     const struct bt_uuid *uuid)
{
	struct bt_uuid_16 *uuid_16 = BT_UUID_16(uuid);

	__ASSERT_NO_MSG(uuid->type == BT_UUID_TYPE_16);

	uuid_16 = user_data_store(dm, uuid_16, sizeof(*uuid_16));
	if (!uuid_16) {
		LOG_ERR("Could not store ATT UUID.");

		return NULL;
	}

	return &uuid_16->uuid;
}

static struct bt_uuid *uuid_128_store(struct bt_gatt_dm *dm,
				      const struct bt_uuid *uuid)
{
	struct bt_uuid_128 *uuid_128 = BT_UUID_128(uuid);

	__ASSERT_NO_MSG(uuid->type == BT_UUID_TYPE_128);

	uuid_128 = user_data_store(dm, uuid_128, sizeof(*uuid_128));

	if (!uuid_128) {
		LOG_ERR("Could not store ATT UUID.");

		return NULL;
	}

	return &uuid_128->uuid;
}

static struct bt_uuid *uuid_store(struct bt_gatt_dm *dm,
				  const struct bt_uuid *uuid)
{
	if (!uuid) {
		LOG_ERR("Uninitialized UUID.");

		return NULL;
	}

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		return uuid_16_store(dm, uuid);
	case BT_UUID_TYPE_128:
		return uuid_128_store(dm, uuid);
	default:
		LOG_ERR("Unsupported UUID type.");
		return NULL;
	}
}

static void discovery_complete(struct bt_gatt_dm *dm)
{
	LOG_DBG("Discovery complete.");
	atomic_set_bit(dm->state_flags, STATE_ATTRS_RELEASE_PENDING);
	if (dm->callback->completed) {
		dm->callback->completed(dm, dm->context);
	}
}

static void discovery_complete_not_found(struct bt_gatt_dm *dm)
{
	LOG_DBG("Discover complete. No service found.");

	svc_attr_memory_release(dm);
	atomic_clear_bit(dm->state_flags, STATE_ATTRS_LOCKED);

	if (dm->callback->service_not_found) {
		dm->callback->service_not_found(dm->conn, dm->context);
	}
}

static void discovery_complete_error(struct bt_gatt_dm *dm, int err)
{
	svc_attr_memory_release(dm);
	atomic_clear_bit(dm->state_flags, STATE_ATTRS_LOCKED);
	if (dm->callback->error_found) {
		dm->callback->error_found(dm->conn, err, dm->context);
	}
}

static u8_t discovery_process_service(struct bt_gatt_dm *dm,
				      const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		discovery_complete_not_found(dm);
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_service_val *service_val = attr->user_data;
	struct bt_gatt_attr *cur_attr = attr_store(dm, attr);

	if (!cur_attr) {
		LOG_ERR("Not enough memory for service attribute.");
		discovery_complete_error(dm, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}
	LOG_DBG("Service detected, handles range: <%u, %u>",
		attr->handle + 1,
		service_val->end_handle);

	cur_attr->uuid = uuid_store(dm, attr->uuid);
	service_val = user_data_store(dm, service_val, sizeof(*service_val));
	if (service_val) {
		service_val->uuid = uuid_store(dm, service_val->uuid);
		cur_attr->user_data = service_val;
	}

	if (!cur_attr->uuid || !service_val ||
	    !service_val->uuid) {
		LOG_ERR("Not enough memory for service attribute data.");
		discovery_complete_error(dm, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	dm->discover_params.uuid         = NULL;
	dm->discover_params.type         = BT_GATT_DISCOVER_ATTRIBUTE;
	dm->discover_params.start_handle = attr->handle + 1;
	dm->discover_params.end_handle   = service_val->end_handle;
	LOG_DBG("Starting descriptors discovery");
	err = bt_gatt_discover(dm->conn, &(dm->discover_params));

	if (err) {
		LOG_ERR("Descriptor discover failed, error: %d.", err);
		discovery_complete_error(dm, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static u8_t discovery_process_attribute(struct bt_gatt_dm *dm,
					 const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	if (!attr) {
		if (dm->cur_attr_id > 1) {
			LOG_DBG("Starting characteristic discovery");
			dm->discover_params.start_handle =
				dm->attrs[0].handle + 1;
			dm->discover_params.type =
				BT_GATT_DISCOVER_CHARACTERISTIC;
			int err = bt_gatt_discover(dm->conn,
						   &(dm->discover_params));

			if (err) {
				LOG_ERR("Characteristic discover failed,"
					" error: %d.",
					err);
				discovery_complete_error(dm, err);
			}
		} else {
			discovery_complete(dm);
		}
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_attr *cur_attr = attr_store(dm, attr);

	if (!cur_attr) {
		LOG_ERR("Not enough memory for next attribute descriptor"
			" at handle %u.",
			attr->handle);
		discovery_complete_error(dm, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}
	cur_attr->uuid = uuid_store(dm, attr->uuid);
	if (!cur_attr->uuid) {
		LOG_ERR("Not enough memory for UUID at handle %u.",
			attr->handle);
		discovery_complete_error(dm, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static u8_t discovery_process_characteristic(
		struct bt_gatt_dm *dm,
		const struct bt_gatt_attr *attr,
		struct bt_gatt_discover_params *params)
{
	if (!attr) {
		discovery_complete(dm);
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_attr *cur_attr = attr_find_by_handle(dm, attr->handle);
	struct bt_gatt_chrc *gatt_chrc;

	if (!cur_attr) {
		/* We should never be here is the server is working properly */
		discovery_complete_error(dm, -ESRCH);
		return BT_GATT_ITER_STOP;
	}

	gatt_chrc = attr->user_data;
	gatt_chrc = user_data_store(dm, gatt_chrc, sizeof(*gatt_chrc));
	if (gatt_chrc) {
		gatt_chrc->uuid = uuid_store(dm, gatt_chrc->uuid);
		cur_attr->user_data = gatt_chrc;
	}

	if (!gatt_chrc || !gatt_chrc->uuid) {
		discovery_complete_error(dm, -ENOMEM);
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

	if (conn != bt_gatt_dm_inst.conn) {
		LOG_ERR("Unexpected conn object. Aborting.");
		discovery_complete_error(&bt_gatt_dm_inst, -EFAULT);
		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
	case BT_GATT_DISCOVER_SECONDARY:
		return discovery_process_service(&bt_gatt_dm_inst,
						 attr, params);
	case BT_GATT_DISCOVER_ATTRIBUTE:
		return discovery_process_attribute(&bt_gatt_dm_inst,
						   attr, params);
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		return discovery_process_characteristic(&bt_gatt_dm_inst,
							attr,
							params);
	default:
		/* This should not be possible */
		__ASSERT(false, "Unknown param type.");
		break;
	}

	return BT_GATT_ITER_STOP;
}

struct bt_gatt_service_val *bt_gatt_dm_attr_service_val(
	const struct bt_gatt_attr *attr)
{
	if ((!bt_uuid_cmp(BT_UUID_GATT_PRIMARY, attr->uuid)) ||
	    (!bt_uuid_cmp(BT_UUID_GATT_SECONDARY, attr->uuid))) {
		return attr->user_data;
	}
	return NULL;
}

struct bt_gatt_chrc *bt_gatt_dm_attr_chrc_val(
	const struct bt_gatt_attr *attr)
{
	if (!bt_uuid_cmp(BT_UUID_GATT_CHRC, attr->uuid)) {
		return attr->user_data;
	}
	return NULL;
}

struct bt_conn *bt_gatt_dm_conn_get(struct bt_gatt_dm *dm)
{
	return dm->conn;
}

size_t bt_gatt_dm_attr_cnt(const struct bt_gatt_dm *dm)
{
	return dm->cur_attr_id;
}

const struct bt_gatt_attr *bt_gatt_dm_service_get(const struct bt_gatt_dm *dm)
{
	return &(dm->attrs[0]);
}

const struct bt_gatt_attr *bt_gatt_dm_char_next(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_attr *prev)
{
	if (!prev) {
		prev = dm->attrs;
	}

	if (dm->attrs <= prev) {
		const struct bt_gatt_attr *const end =
			&(dm->attrs[dm->cur_attr_id]);
		while (++prev < end) {
			if (!bt_uuid_cmp(BT_UUID_GATT_CHRC, prev->uuid)) {
				return prev;
			}
		}
	}

	return NULL;
}

const struct bt_gatt_attr *bt_gatt_dm_char_by_uuid(
	const struct bt_gatt_dm *dm,
	const struct bt_uuid *uuid)
{
	const struct bt_gatt_attr *curr = NULL;

	while ((curr = bt_gatt_dm_char_next(dm, curr)) != NULL) {
		struct bt_gatt_chrc *chrc = bt_gatt_dm_attr_chrc_val(curr);

		__ASSERT_NO_MSG(chrc != NULL);
		if (!bt_uuid_cmp(uuid, chrc->uuid)) {
			return curr;
		}
	}
	return NULL;
}

const struct bt_gatt_attr *bt_gatt_dm_attr_by_handle(
	const struct bt_gatt_dm *dm,
	u16_t handle)
{
	return attr_find_by_handle_c(dm, handle);
}

const struct bt_gatt_attr *bt_gatt_dm_attr_next(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_attr *prev)
{
	if (!prev) {
		prev = dm->attrs;
	}

	if (dm->attrs <= prev) {
		const struct bt_gatt_attr *const end =
			&(dm->attrs[dm->cur_attr_id]);
		if (++prev < end) {
			return prev;
		}
	}
	return NULL;
}

const struct bt_gatt_attr *bt_gatt_dm_desc_by_uuid(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_attr *attr_chrc,
	const struct bt_uuid *uuid)
{
	const struct bt_gatt_attr *curr = attr_chrc;

	while ((curr = bt_gatt_dm_desc_next(dm, curr)) != NULL) {
		if (!bt_uuid_cmp(uuid, curr->uuid)) {
			break;
		}
	}
	return curr;
}

const struct bt_gatt_attr *bt_gatt_dm_desc_next(
	const struct bt_gatt_dm *dm,
	const struct bt_gatt_attr *prev)
{
	const struct bt_gatt_attr *curr = bt_gatt_dm_attr_next(dm, prev);

	if (curr && !bt_uuid_cmp(BT_UUID_GATT_CHRC, curr->uuid)) {
		curr = NULL;
	}
	return curr;
}


int bt_gatt_dm_start(struct bt_conn *conn,
		     const struct bt_uuid *svc_uuid,
		     const struct bt_gatt_dm_cb *cb,
		     void *context)
{
	int err;
	struct bt_gatt_dm *dm;

	if (svc_uuid &&
	    (svc_uuid->type != BT_UUID_TYPE_16) &&
	    (svc_uuid->type != BT_UUID_TYPE_128)) {
		return -EINVAL;
	}

	if (!cb) {
		return -EINVAL;
	}

	dm = &bt_gatt_dm_inst;

	if (atomic_test_and_set_bit(dm->state_flags, STATE_ATTRS_LOCKED)) {
		return -EALREADY;
	}

	dm->conn = conn;
	dm->context = context;
	dm->callback = cb;
	dm->cur_attr_id = 0;
	dm->data_chunk.cur_len = 0;
	dm->data_chunk.cur_id = 0;

	dm->discover_params.uuid = svc_uuid ? uuid_store(dm, svc_uuid) : NULL;
	dm->discover_params.func = discovery_callback;
	dm->discover_params.start_handle = 0x0001;
	dm->discover_params.end_handle = 0xffff;
	dm->discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(conn, &dm->discover_params);
	if (err) {
		LOG_ERR("Discover failed, error: %d.", err);
		atomic_clear_bit(dm->state_flags, STATE_ATTRS_LOCKED);
	}

	return err;
}

int bt_gatt_dm_continue(struct bt_gatt_dm *dm, void *context)
{
	int err;

	if ((!dm) ||
	    (!dm->callback) ||
	    (dm->discover_params.func != discovery_callback)) {
		return -EINVAL;
	}

	/* If UUID is set, it does not make sense to call this function.
	 * The stored UUID would be broken anyway in bt_gatt_dm_data_release.
	 */
	if (dm->discover_params.uuid) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(dm->state_flags, STATE_ATTRS_LOCKED)) {
		return -EALREADY;
	}

	if (dm->discover_params.end_handle == 0xffff) {
		/* No more handles to discover. */
		discovery_complete_not_found(dm);
		return 0;
	}

	dm->context = context;
	dm->discover_params.start_handle = dm->discover_params.end_handle + 1;
	dm->discover_params.end_handle = 0xffff;
	dm->discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(dm->conn, &dm->discover_params);
	if (err) {
		LOG_ERR("Discover failed, error: %d.", err);
		atomic_clear_bit(dm->state_flags, STATE_ATTRS_LOCKED);
	}

	return err;
}

int bt_gatt_dm_data_release(struct bt_gatt_dm *dm)
{
	if (!atomic_test_and_clear_bit(dm->state_flags,
				       STATE_ATTRS_RELEASE_PENDING)) {
		return -EALREADY;
	}

	svc_attr_memory_release(dm);
	atomic_clear_bit(dm->state_flags, STATE_ATTRS_LOCKED);

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

static void attr_print(const struct bt_gatt_dm *dm,
		       const struct bt_gatt_attr *attr)
{
	char str[UUID_STR_LEN];

	bt_uuid_to_str(attr->uuid, str, sizeof(str));
	printk("ATT[%u]: \tUUID: 0x%s\tHandle: 0x%04X\tValue:\n",
	       (unsigned int)(attr - dm->attrs), str, attr->handle);

	if ((bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) == 0) ||
	    (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY) == 0)) {
		svc_attr_data_print(attr->user_data);
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
		chrc_attr_data_print(attr->user_data);
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
		printk("\tCCCD\n");
	}
}

void bt_gatt_dm_data_print(const struct bt_gatt_dm *dm)
{
	const struct bt_gatt_attr *attr = NULL;

	while (NULL != (attr = bt_gatt_dm_attr_next(dm, attr))) {
		attr_print(dm, attr);
	}
}

#endif /* CONFIG_BT_GATT_DM_DATA_PRINT */
