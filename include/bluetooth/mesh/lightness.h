/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_lightness Light Lightness Models
 * @{
 * @brief API for the Light Lightness models.
 */

#ifndef BT_MESH_LIGHTNESS_H__
#define BT_MESH_LIGHTNESS_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Minimum permitted Lightness level */
#define BT_MESH_LIGHTNESS_MIN 0
/** Maximum permitted Lightness level */
#define BT_MESH_LIGHTNESS_MAX UINT16_MAX

/** Lightness set message parameters. */
struct bt_mesh_lightness_set {
	/** Lightness level. */
	uint16_t lvl;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Lightness status message parameters. */
struct bt_mesh_lightness_status {
	/** Current Lightness level. */
	uint16_t current;
	/** Target Lightness level. */
	uint16_t target;
	/**
	 * Time remaining of the ongoing transition in milliseconds, or
	 * @em SYS_FOREVER_MS. If there's no ongoing transition,
	 * @c remaining_time is 0.
	 */
	int32_t remaining_time;
};

/** Lightness range parameters. */
struct bt_mesh_lightness_range {
	uint16_t min; /**< Minimum allowed level. */
	uint16_t max; /**< Maximum allowed level. */
};

/** Lightness range message parameters. */
struct bt_mesh_lightness_range_status {
	/** Status of the previous operation. */
	enum bt_mesh_model_status status;
	/** Current Lightness range. */
	struct bt_mesh_lightness_range range;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LIGHTNESS_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x4B)
#define BT_MESH_LIGHTNESS_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x4C)
#define BT_MESH_LIGHTNESS_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x4D)
#define BT_MESH_LIGHTNESS_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x4E)
#define BT_MESH_LIGHTNESS_OP_LINEAR_GET BT_MESH_MODEL_OP_2(0x82, 0x4F)
#define BT_MESH_LIGHTNESS_OP_LINEAR_SET BT_MESH_MODEL_OP_2(0x82, 0x50)
#define BT_MESH_LIGHTNESS_OP_LINEAR_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x51)
#define BT_MESH_LIGHTNESS_OP_LINEAR_STATUS BT_MESH_MODEL_OP_2(0x82, 0x52)
#define BT_MESH_LIGHTNESS_OP_LAST_GET BT_MESH_MODEL_OP_2(0x82, 0x53)
#define BT_MESH_LIGHTNESS_OP_LAST_STATUS BT_MESH_MODEL_OP_2(0x82, 0x54)
#define BT_MESH_LIGHTNESS_OP_DEFAULT_GET BT_MESH_MODEL_OP_2(0x82, 0x55)
#define BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS BT_MESH_MODEL_OP_2(0x82, 0x56)
#define BT_MESH_LIGHTNESS_OP_RANGE_GET BT_MESH_MODEL_OP_2(0x82, 0x57)
#define BT_MESH_LIGHTNESS_OP_RANGE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x58)
#define BT_MESH_LIGHTNESS_OP_DEFAULT_SET BT_MESH_MODEL_OP_2(0x82, 0x59)
#define BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x5A)
#define BT_MESH_LIGHTNESS_OP_RANGE_SET BT_MESH_MODEL_OP_2(0x82, 0x5B)
#define BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x5C)

#define BT_MESH_LIGHTNESS_MSG_LEN_GET 0
#define BT_MESH_LIGHTNESS_MSG_MINLEN_SET 3
#define BT_MESH_LIGHTNESS_MSG_MAXLEN_SET 5
#define BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS 2
#define BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS 5
#define BT_MESH_LIGHTNESS_MSG_LEN_LAST_GET 0
#define BT_MESH_LIGHTNESS_MSG_LEN_LAST_STATUS 2
#define BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_GET 0
#define BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS 2
#define BT_MESH_LIGHTNESS_MSG_LEN_RANGE_GET 0
#define BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS 5
#define BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET 2
#define BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET 4
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHTNESS_H__ */

/** @} */
