/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <init.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/ots.h>
#include "ots_internal.h"
#include "ots_obj_manager_internal.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_gatt_ots, CONFIG_BT_GATT_OTS_LOG_LEVEL);

static ssize_t ots_feature_read(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				u16_t len, u16_t offset)
{
	struct bt_gatt_ots *ots = (struct bt_gatt_ots *) attr->user_data;

	LOG_DBG("OTS Feature GATT Read Operation");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ots->features,
		sizeof(ots->features));
}

static ssize_t ots_obj_name_read(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	struct bt_gatt_ots *ots = (struct bt_gatt_ots *) attr->user_data;

	LOG_DBG("OTS Object Name GATT Read Operation");

	if (!ots->cur_obj) {
		LOG_DBG("No Current Object selected in OTS!");
		return BT_GATT_ERR(BT_GATT_OTS_OBJECT_NOT_SELECTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 ots->cur_obj->metadata.name,
				 strlen(ots->cur_obj->metadata.name) + 1);
}

static ssize_t ots_obj_type_read(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	struct bt_gatt_ots *ots = (struct bt_gatt_ots *) attr->user_data;
	struct bt_gatt_ots_obj_metadata *obj_meta;

	LOG_DBG("OTS Object Type GATT Read Operation");

	if (!ots->cur_obj) {
		LOG_DBG("No Current Object selected in OTS!");
		return BT_GATT_ERR(BT_GATT_OTS_OBJECT_NOT_SELECTED);
	}

	obj_meta = &ots->cur_obj->metadata;
	if (obj_meta->type.uuid.type == BT_UUID_TYPE_128) {
		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 obj_meta->type.uuid_128.val,
					 sizeof(obj_meta->type.uuid_128.val));
	} else {
		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &obj_meta->type.uuid_16.val,
					 sizeof(obj_meta->type.uuid_16.val));
	}
}

static ssize_t ots_obj_size_read(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	struct bt_gatt_ots *ots = (struct bt_gatt_ots *) attr->user_data;

	LOG_DBG("OTS Object Size GATT Read Operation");

	if (!ots->cur_obj) {
		LOG_DBG("No Current Object selected in OTS!");
		return BT_GATT_ERR(BT_GATT_OTS_OBJECT_NOT_SELECTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &ots->cur_obj->metadata.size,
				 sizeof(ots->cur_obj->metadata.size));
}

static ssize_t ots_obj_id_read(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       u16_t len, u16_t offset)
{
	struct bt_gatt_ots *ots = (struct bt_gatt_ots *) attr->user_data;
	struct bt_gatt_ots_obj_id id;

	LOG_DBG("OTS Object ID GATT Read Operation");

	if (!ots->cur_obj) {
		LOG_DBG("No Current Object selected in OTS!");
		return BT_GATT_ERR(BT_GATT_OTS_OBJECT_NOT_SELECTED);
	}

	id.val = ots->cur_obj->metadata.id;
	LOG_HEXDUMP_DBG(id.ba, sizeof(id.ba), "Current Object ID:");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, id.ba,
				 sizeof(id.ba));
}

static ssize_t ots_obj_prop_read(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	struct bt_gatt_ots *ots = (struct bt_gatt_ots *) attr->user_data;

	LOG_DBG("OTS Object Properties GATT Read Operation");

	if (!ots->cur_obj) {
		LOG_DBG("No Current Object selected in OTS!");
		return BT_GATT_ERR(BT_GATT_OTS_OBJECT_NOT_SELECTED);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &ots->cur_obj->metadata.props,
				 sizeof(ots->cur_obj->metadata.props));
}

int bt_gatt_ots_object_add(struct bt_gatt_ots *ots,
			   struct bt_gatt_ots_obj_init *obj_init)
{
	int err;
	struct bt_gatt_ots_object *obj;

	err = bt_gatt_ots_obj_manager_obj_add(ots->obj_manager, &obj);
	if (err) {
		LOG_ERR("No space available in the object manager");
		return err;
	}

	/* Initialize object. */
	obj->metadata.name = obj_init->name;
	obj->metadata.type = obj_init->type;
	obj->metadata.size.alloc = obj_init->size;
	obj->metadata.size.cur = obj_init->size;
	obj->metadata.props = obj_init->props;

	/* Request object data. */
	if (ots->cb->obj_created) {
		err = ots->cb->obj_created(ots, NULL, obj->metadata.id,
					   obj_init);
		if (err) {
			bt_gatt_ots_obj_manager_obj_delete(obj);
			return err;
		}
	}

	/* Make object the Current Object if this is the first one added. */
	if (!ots->cur_obj) {
		ots->cur_obj = obj;
	}

	return 0;
}

int bt_gatt_ots_object_delete(struct bt_gatt_ots *ots, u64_t id)
{
	int err;
	struct bt_gatt_ots_object *obj;

	err = bt_gatt_ots_obj_manager_obj_get(ots->obj_manager, id, &obj);
	if (err) {
		return err;
	}

	if (ots->cur_obj == obj) {
		ots->cur_obj = NULL;
	}

	err = bt_gatt_ots_obj_manager_obj_delete(obj);
	if (err) {
		return err;
	}

	if (ots->cb->obj_deleted) {
		ots->cb->obj_deleted(ots, NULL, obj->metadata.id);
	}

	return 0;
}

#if defined(CONFIG_BT_GATT_OTS_SECONDARY_SVC)
struct bt_gatt_attr *bt_gatt_ots_svc_get(struct bt_gatt_ots *ots)
{
	return &ots->service->attrs[0];
}
#endif

static u32_t ots_oacp_supported_features_get(void)
{
	u32_t oacp_ref = 0;

	if (IS_ENABLED(CONFIG_BT_GATT_OTS_OACP_READ_SUPPORT)) {
		BT_GATT_OTS_OACP_SET_FEAT_READ(oacp_ref);
	}

	return oacp_ref;
}

static u32_t ots_olcp_supported_features_get(void)
{
	u32_t olcp_ref = 0;

	if (IS_ENABLED(CONFIG_BT_GATT_OTS_OLCP_GO_TO_SUPPORT)) {
		BT_GATT_OTS_OLCP_SET_FEAT_GO_TO(olcp_ref);
	}

	return olcp_ref;
}

int bt_gatt_ots_init(struct bt_gatt_ots *ots,
		     struct bt_gatt_ots_init *ots_init)
{
	int err;
	u32_t oacp_ref;
	u32_t olcp_ref;

	if (!ots || !ots_init || !ots_init->cb) {
		return -EINVAL;
	}

	/* Set callback structure. */
	ots->cb = ots_init->cb;

	/* Check OACP supported features against Kconfig. */
	oacp_ref = ots_oacp_supported_features_get();
	if (ots_init->features.oacp & (~oacp_ref)) {
		return -ENOTSUP;
	}
	ots->features.oacp = ots_init->features.oacp;
	LOG_DBG("OACP features: 0x%04X", ots->features.oacp);

	/* Check OLCP supported features against Kconfig. */
	olcp_ref = ots_olcp_supported_features_get();
	if (ots_init->features.olcp & (~olcp_ref)) {
		return -ENOTSUP;
	}
	ots->features.olcp = ots_init->features.olcp;
	LOG_DBG("OLCP features: 0x%04X", ots->features.olcp);

	/* Register L2CAP context. */
	err = bt_gatt_ots_l2cap_register(&ots->l2cap);
	if (err) {
		return err;
	}

	err = bt_gatt_service_register(ots->service);
	if (err) {
		return err;
	}

	LOG_DBG("Initialized OTS");

	return 0;
}

#if defined(CONFIG_BT_GATT_OTS_SECONDARY_SVC)
	#define BT_GATT_OTS_SERVICE	BT_GATT_SECONDARY_SERVICE
#else
	#define BT_GATT_OTS_SERVICE	BT_GATT_PRIMARY_SERVICE
#endif

#define BT_GATT_OTS_ATTRS(_ots)	{					\
	BT_GATT_OTS_SERVICE(BT_UUID_OTS),				\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_FEATURE,			\
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,			\
		ots_feature_read, NULL, &_ots),				\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_NAME,			\
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,			\
		ots_obj_name_read, NULL, &_ots),			\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_TYPE,			\
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,			\
		ots_obj_type_read, NULL, &_ots),			\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_SIZE,			\
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,			\
		ots_obj_size_read, NULL, &_ots),			\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_ID,				\
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,			\
		ots_obj_id_read, NULL, &_ots),				\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_PROPERTIES,			\
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,			\
		ots_obj_prop_read, NULL, &_ots),			\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_ACTION_CP,			\
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,		\
		BT_GATT_PERM_WRITE, NULL,				\
		bt_gatt_ots_oacp_write, &_ots),				\
	BT_GATT_CCC_MANAGED(&_ots.oacp_ind.ccc,				\
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),		\
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_LIST_CP,			\
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,		\
		BT_GATT_PERM_WRITE, NULL,				\
		bt_gatt_ots_olcp_write, &_ots),				\
	BT_GATT_CCC_MANAGED(&_ots.olcp_ind.ccc,				\
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)			\
}

#define BT_GATT_OTS_ATTRS_DEF(n, _ots_instances)			\
	static struct bt_gatt_attr ots_attrs_##n[] =			\
		BT_GATT_OTS_ATTRS(_ots_instances[n]);

#define BT_GATT_OTS_SVC_ITEM(n, _) BT_GATT_SERVICE(ots_attrs_##n),

#define BT_GATT_OTS_INSTANCE_LIST_DEF(_instance_num)			\
	static struct bt_gatt_ots ots_instances[_instance_num];		\
	UTIL_EVAL(UTIL_REPEAT(						\
		_instance_num, BT_GATT_OTS_ATTRS_DEF, ots_instances))	\
	static struct bt_gatt_service ots_service_list[] = {		\
		UTIL_LISTIFY(_instance_num, BT_GATT_OTS_SVC_ITEM)	\
	}

#define BT_GATT_OTS_INSTANCE_LIST_SIZE	(ARRAY_SIZE(ots_instances))
#define BT_GATT_OTS_INSTANCE_LIST_START	ots_instances
#define BT_GATT_OTS_INSTANCE_LIST_END	\
	(&ots_instances[BT_GATT_OTS_INSTANCE_LIST_SIZE])

#define BT_GATT_OTS_SERVICE_LIST_START	ots_service_list

BT_GATT_OTS_INSTANCE_LIST_DEF(CONFIG_BT_GATT_OTS_MAX_INST_CNT);
static u32_t instance_cnt;

struct bt_gatt_ots *bt_gatt_ots_instance_get(void)
{
	if (instance_cnt >= BT_GATT_OTS_INSTANCE_LIST_SIZE) {
		return NULL;
	}

	return &BT_GATT_OTS_INSTANCE_LIST_START[instance_cnt++];
}

static int bt_gatt_ots_instances_prepare(struct device *dev)
{
	u32_t index;
	struct bt_gatt_ots *instance;

	for (instance = BT_GATT_OTS_INSTANCE_LIST_START, index = 0;
	     instance != BT_GATT_OTS_INSTANCE_LIST_END;
	     instance++, index++) {
		/* Assign an object pool to the OTS instance. */
		instance->obj_manager = bt_gatt_ots_obj_manager_assign();
		if (!instance->obj_manager) {
			LOG_ERR("OTS Object manager instance not available");
			return -ENOMEM;
		}

		/* Assign pointer to the service descriptor. */
		instance->service = &BT_GATT_OTS_SERVICE_LIST_START[index];

		/* Initialize CCC descriptors for characteristics with
		 * indication properties.
		 */
		instance->oacp_ind.ccc.cfg_changed =
			bt_gatt_ots_oacp_cfg_changed;
		instance->olcp_ind.ccc.cfg_changed =
			bt_gatt_ots_olcp_cfg_changed;
	}

	return 0;
}

SYS_INIT(bt_gatt_ots_instances_prepare, APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
