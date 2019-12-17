/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_prop Generic Property models
 * @{
 * @brief API for the Generic Property models.
 */

#ifndef BT_MESH_GEN_PROP_H__
#define BT_MESH_GEN_PROP_H__

#include <bluetooth/mesh.h>
#include <bluetooth/mesh/properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_PROP_OP_MFR_PROPS_GET BT_MESH_MODEL_OP_2(0x82, 0x2A)
#define BT_MESH_PROP_OP_MFR_PROPS_STATUS BT_MESH_MODEL_OP_1(0x43)
#define BT_MESH_PROP_OP_MFR_PROP_GET BT_MESH_MODEL_OP_2(0x82, 0x2B)
#define BT_MESH_PROP_OP_MFR_PROP_SET BT_MESH_MODEL_OP_1(0x44)
#define BT_MESH_PROP_OP_MFR_PROP_SET_UNACK BT_MESH_MODEL_OP_1(0x45)
#define BT_MESH_PROP_OP_MFR_PROP_STATUS BT_MESH_MODEL_OP_1(0x46)
#define BT_MESH_PROP_OP_ADMIN_PROPS_GET BT_MESH_MODEL_OP_2(0x82, 0x2C)
#define BT_MESH_PROP_OP_ADMIN_PROPS_STATUS BT_MESH_MODEL_OP_1(0x47)
#define BT_MESH_PROP_OP_ADMIN_PROP_GET BT_MESH_MODEL_OP_2(0x82, 0x2D)
#define BT_MESH_PROP_OP_ADMIN_PROP_SET BT_MESH_MODEL_OP_1(0x48)
#define BT_MESH_PROP_OP_ADMIN_PROP_SET_UNACK BT_MESH_MODEL_OP_1(0x49)
#define BT_MESH_PROP_OP_ADMIN_PROP_STATUS BT_MESH_MODEL_OP_1(0x4A)
#define BT_MESH_PROP_OP_USER_PROPS_GET BT_MESH_MODEL_OP_2(0x82, 0x2E)
#define BT_MESH_PROP_OP_USER_PROPS_STATUS BT_MESH_MODEL_OP_1(0x4B)
#define BT_MESH_PROP_OP_USER_PROP_GET BT_MESH_MODEL_OP_2(0x82, 0x2F)
#define BT_MESH_PROP_OP_USER_PROP_SET BT_MESH_MODEL_OP_1(0x4C)
#define BT_MESH_PROP_OP_USER_PROP_SET_UNACK BT_MESH_MODEL_OP_1(0x4D)
#define BT_MESH_PROP_OP_USER_PROP_STATUS BT_MESH_MODEL_OP_1(0x4E)
#define BT_MESH_PROP_OP_CLIENT_PROPS_GET BT_MESH_MODEL_OP_1(0x4F)
#define BT_MESH_PROP_OP_CLIENT_PROPS_STATUS BT_MESH_MODEL_OP_1(0x50)

#define BT_MESH_PROP_MSG_LEN_PROPS_GET 0
#define BT_MESH_PROP_MSG_LEN_CLIENT_PROPS_GET 2
#define BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS 0
#define BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS (2 * CONFIG_BT_MESH_PROP_MAXCOUNT)
#define BT_MESH_PROP_MSG_LEN_PROP_GET 2
#define BT_MESH_PROP_MSG_MINLEN_PROP_STATUS 3
#define BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS (3 + CONFIG_BT_MESH_PROP_MAXSIZE)
#define BT_MESH_PROP_MSG_LEN_MFR_PROP_SET 3
#define BT_MESH_PROP_MSG_MINLEN_ADMIN_PROP_SET 3
#define BT_MESH_PROP_MSG_MAXLEN_ADMIN_PROP_SET (3 + CONFIG_BT_MESH_PROP_MAXSIZE)
#define BT_MESH_PROP_MSG_MINLEN_USER_PROP_SET 2
#define BT_MESH_PROP_MSG_MAXLEN_USER_PROP_SET (2 + CONFIG_BT_MESH_PROP_MAXSIZE)
#define BT_MESH_PROP_MSG_MAXLEN(_prop_cnt)                                     \
	MAX(BT_MESH_MODEL_BUF_LEN(BT_MESH_PROP_OP_MFR_PROP_STATUS,             \
				  BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS),        \
	    BT_MESH_MODEL_BUF_LEN(BT_MESH_PROP_OP_MFR_PROPS_STATUS,            \
				  2 * (_prop_cnt)))
/** @endcond */

/** Access flags for properties */
enum bt_mesh_prop_access {
	/** Access to the property is prohibited. */
	BT_MESH_PROP_ACCESS_PROHIBITED = 0,
	/** Property may be read. */
	BT_MESH_PROP_ACCESS_READ = BIT(0),
	/** Property may be written. */
	BT_MESH_PROP_ACCESS_WRITE = BIT(1),
	/** Property may be read or written. */
	BT_MESH_PROP_ACCESS_READ_WRITE =
		(BT_MESH_PROP_ACCESS_READ | BT_MESH_PROP_ACCESS_WRITE),
};

/** Property Server kinds. */
enum bt_mesh_prop_srv_kind {
	/** Manufacturer property server. */
	BT_MESH_PROP_SRV_KIND_MFR,
	/** Admin property server. */
	BT_MESH_PROP_SRV_KIND_ADMIN,
	/** User property server. */
	BT_MESH_PROP_SRV_KIND_USER,
	/** Client property server. */
	BT_MESH_PROP_SRV_KIND_CLIENT,
};

/** Property representation. */
struct bt_mesh_prop {
	/** Property ID. @sa bt_mesh_property_ids. */
	u16_t id;
	/** User access flags for the property. */
	enum bt_mesh_prop_access user_access;
};

/** Property value representation. */
struct bt_mesh_prop_val {
	struct bt_mesh_prop meta; /**< Metadata for this property. */
	size_t size; /**< Size of the property value. */
	u8_t *value; /**< Property value. */
};

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PROP_H__ */

/** @} */
