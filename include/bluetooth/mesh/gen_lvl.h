/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_lvl Bluetooth Mesh Generic Level models
 * @{
 * @brief Common API for the Bluetooth Mesh Generic Level models.
 */
#ifndef BT_MESH_GEN_LVL_H__
#define BT_MESH_GEN_LVL_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Generic Level minimum value. */
#define BT_MESH_LVL_MIN INT16_MIN
/** Generic Level maximum value. */
#define BT_MESH_LVL_MAX INT16_MAX

/** Generic Level set message parameters.  */
struct bt_mesh_lvl_set {
	/** New level. */
	int16_t lvl;
	/** Whether this is a new action. */
	bool new_transaction;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Generic Level delta set message parameters.  */
struct bt_mesh_lvl_delta_set {
	/** Translation from original value. */
	int32_t delta;
	/**
	 * Whether this is a new transaction. If true, the delta should be
	 * relative to the current value. If false, the delta should be
	 * relative to the original value in the previous delta_set command.
	 */
	bool new_transaction;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Generic Level move set message parameters. */
struct bt_mesh_lvl_move_set {
	/** Translation to make for every transition step. */
	int16_t delta;
	/** Whether this is a new action. */
	bool new_transaction;
	/**
	 * Transition parameters, or NULL. @c delay indicates time until the
	 * move should start, @c time indicates the amount of time each @c delta
	 * step should take.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Generic Level status message parameters.  */
struct bt_mesh_lvl_status {
	/** Current level value. */
	int16_t current;
	/**
	 * Target value for the ongoing transition. If there's no ongoing
	 * transition, @c target should match @c value.
	 */
	int16_t target;
	/**
	 * Time remaining of the ongoing transition (in milliseconds),
	 * or @em SYS_FOREVER_MS. If there's no ongoing transition,
	 * @c remaining_time is 0.
	 */
	int32_t remaining_time;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LVL_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x05)
#define BT_MESH_LVL_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x06)
#define BT_MESH_LVL_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x07)
#define BT_MESH_LVL_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x08)
#define BT_MESH_LVL_OP_DELTA_SET BT_MESH_MODEL_OP_2(0x82, 0x09)
#define BT_MESH_LVL_OP_DELTA_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x0A)
#define BT_MESH_LVL_OP_MOVE_SET BT_MESH_MODEL_OP_2(0x82, 0x0B)
#define BT_MESH_LVL_OP_MOVE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x0C)

#define BT_MESH_LVL_MSG_LEN_GET 0
#define BT_MESH_LVL_MSG_MINLEN_SET 3
#define BT_MESH_LVL_MSG_MAXLEN_SET 5
#define BT_MESH_LVL_MSG_MINLEN_STATUS 2
#define BT_MESH_LVL_MSG_MAXLEN_STATUS 5
#define BT_MESH_LVL_MSG_MINLEN_DELTA_SET 5
#define BT_MESH_LVL_MSG_MAXLEN_DELTA_SET 7
#define BT_MESH_LVL_MSG_MINLEN_MOVE_SET 3
#define BT_MESH_LVL_MSG_MAXLEN_MOVE_SET 5
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LVL_H__ */

/** @} */
