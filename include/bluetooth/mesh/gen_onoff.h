/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_onoff Generic OnOff Models
 * @{
 * @brief Common API for the Generic OnOff models.
 */

#ifndef BT_MESH_GEN_ONOFF_H__
#define BT_MESH_GEN_ONOFF_H__

#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Mandatory parameters for the Generic OnOff Set message. */
struct bt_mesh_onoff_set {
	/** State to set. */
	bool on_off;
	/** Whether this should reuse the previous transaction identifier. */
	bool reuse_transaction;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Parameters for the Generic OnOff Status message. */
struct bt_mesh_onoff_status {
	/** The present value of the Generic OnOff state. */
	bool present_on_off;
	/** The target value of the Generic OnOff state (optional). */
	bool target_on_off;
	/** Remaining time value in milliseconds. */
	int32_t remaining_time;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_ONOFF_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_ONOFF_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_ONOFF_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_ONOFF_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x04)

#define BT_MESH_ONOFF_MSG_LEN_GET 0
#define BT_MESH_ONOFF_MSG_MINLEN_SET 2
#define BT_MESH_ONOFF_MSG_MAXLEN_SET 4
#define BT_MESH_ONOFF_MSG_MINLEN_STATUS 1
#define BT_MESH_ONOFF_MSG_MAXLEN_STATUS 3
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_ONOFF_H__ */

/** @} */
