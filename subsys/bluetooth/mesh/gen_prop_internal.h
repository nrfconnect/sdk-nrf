/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_prop_internal Generic Property model internals
 * @ingroup bt_mesh_prop
 * @{
 * @brief Common internal API for the Generic Property models.
 */

#ifndef BT_MESH_GEN_PROP_INTERNAL_H__
#define BT_MESH_GEN_PROP_INTERNAL_H__

#include <bluetooth/mesh/gen_prop.h>

/** Common opcode types for lookup */
enum bt_mesh_prop_op_type {
	BT_MESH_PROP_OP_PROPS_GET,
	BT_MESH_PROP_OP_PROPS_STATUS,
	BT_MESH_PROP_OP_PROP_SET,
	BT_MESH_PROP_OP_PROP_GET,
	BT_MESH_PROP_OP_PROP_SET_UNACK,
	BT_MESH_PROP_OP_PROP_STATUS,
};

static inline enum bt_mesh_prop_srv_kind
srv_kind(const struct bt_mesh_model *mod)
{
	switch (mod->id) {
	case BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV:
		return BT_MESH_PROP_SRV_KIND_ADMIN;
	case BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV:
		return BT_MESH_PROP_SRV_KIND_MFR;
	case BT_MESH_MODEL_ID_GEN_USER_PROP_SRV:
		return BT_MESH_PROP_SRV_KIND_USER;
	case BT_MESH_MODEL_ID_GEN_CLIENT_PROP_SRV:
		return BT_MESH_PROP_SRV_KIND_CLIENT;
	}
	return 0;
}

static inline u32_t op_get(enum bt_mesh_prop_op_type op_type,
			   enum bt_mesh_prop_srv_kind kind)
{
	switch (kind) {
	case BT_MESH_PROP_SRV_KIND_MFR:
	case BT_MESH_PROP_SRV_KIND_ADMIN:
	case BT_MESH_PROP_SRV_KIND_USER:
		switch (op_type) {
		case BT_MESH_PROP_OP_PROPS_GET:
			return BT_MESH_PROP_OP_MFR_PROPS_GET + 2 * kind;
		case BT_MESH_PROP_OP_PROP_GET:
			return BT_MESH_PROP_OP_MFR_PROP_GET + 2 * kind;
		case BT_MESH_PROP_OP_PROPS_STATUS:
			return BT_MESH_PROP_OP_MFR_PROPS_STATUS + 4 * kind;
		case BT_MESH_PROP_OP_PROP_SET:
			return BT_MESH_PROP_OP_MFR_PROP_SET + 4 * kind;
		case BT_MESH_PROP_OP_PROP_SET_UNACK:
			return BT_MESH_PROP_OP_MFR_PROP_SET_UNACK + 4 * kind;
		case BT_MESH_PROP_OP_PROP_STATUS:
			return BT_MESH_PROP_OP_MFR_PROP_STATUS + 4 * kind;
		}
		break;
	case BT_MESH_PROP_SRV_KIND_CLIENT:
		switch (op_type) {
		case BT_MESH_PROP_OP_PROPS_GET:
			return BT_MESH_PROP_OP_CLIENT_PROPS_GET;
		case BT_MESH_PROP_OP_PROPS_STATUS:
			return BT_MESH_PROP_OP_CLIENT_PROPS_STATUS;
		default:
			return 0;
		}
		break;
	}
	return 0;
}

#endif /* BT_MESH_GEN_PROP_INTERNAL_H__ */

/** @} */
