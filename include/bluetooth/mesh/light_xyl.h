/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_xyl Light xyL models
 * @{
 * @brief API for the Light xyL models.
 */

#ifndef BT_MESH_LIGHT_XYL_H__
#define BT_MESH_LIGHT_XYL_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Light xy parameters. */
struct bt_mesh_light_xy {
	/** xyL x value */
	uint16_t x;
	/** xyL y value */
	uint16_t y;
};

/** Common light xyL message parameters. */
struct bt_mesh_light_xyl {
	/** Lightness value */
	uint16_t lightness;
	/** xy parameters */
	struct bt_mesh_light_xy xy;
};

/** Light xyL set message parameters. */
struct bt_mesh_light_xyl_set_params {
	/** xyL set parameters */
	struct bt_mesh_light_xyl params;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	struct bt_mesh_model_transition *transition;
};

/** Light xyL set response message parameters. */
struct bt_mesh_light_xyl_rsp {
	/** Current xyL parameters */
	struct bt_mesh_light_xyl current;
	/** Target xyL parameters */
	struct bt_mesh_light_xyl target;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Light xyL status message parameters. */
struct bt_mesh_light_xyl_status {
	/** Status parameters */
	struct bt_mesh_light_xyl params;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Light xy range parameters. */
struct bt_mesh_light_xy_range {
	/** Maximal xy range */
	struct bt_mesh_light_xy min;
	/** Maximal xy range */
	struct bt_mesh_light_xy max;
};

/** Light xyL range status message parameters. */
struct bt_mesh_light_xyl_range_status {
	/** Range status code */
	enum bt_mesh_model_status status_code;
	/** Range status parameters */
	struct bt_mesh_light_xy_range range;
};

/** @cond INTERNAL_HIDDEN */

#define BT_MESH_LIGHT_XYL_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x83)
#define BT_MESH_LIGHT_XYL_OP_SET BT_MESH_MODEL_OP_2(0x82, 0x84)
#define BT_MESH_LIGHT_XYL_OP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x85)
#define BT_MESH_LIGHT_XYL_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x86)
#define BT_MESH_LIGHT_XYL_OP_TARGET_GET BT_MESH_MODEL_OP_2(0x82, 0x87)
#define BT_MESH_LIGHT_XYL_OP_TARGET_STATUS BT_MESH_MODEL_OP_2(0x82, 0x88)
#define BT_MESH_LIGHT_XYL_OP_DEFAULT_GET BT_MESH_MODEL_OP_2(0x82, 0x89)
#define BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS BT_MESH_MODEL_OP_2(0x82, 0x8A)
#define BT_MESH_LIGHT_XYL_OP_RANGE_GET BT_MESH_MODEL_OP_2(0x82, 0x8B)
#define BT_MESH_LIGHT_XYL_OP_RANGE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x8C)
#define BT_MESH_LIGHT_XYL_OP_DEFAULT_SET BT_MESH_MODEL_OP_2(0x82, 0x8D)
#define BT_MESH_LIGHT_XYL_OP_DEFAULT_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x8E)
#define BT_MESH_LIGHT_XYL_OP_RANGE_SET BT_MESH_MODEL_OP_2(0x82, 0x8F)
#define BT_MESH_LIGHT_XYL_OP_RANGE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x90)

#define BT_MESH_LIGHT_XYL_MSG_LEN_GET 0
#define BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT 6
#define BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET 8
#define BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS 9
#define BT_MESH_LIGHT_XYL_MSG_MINLEN_STATUS 6
#define BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS 7
#define BT_MESH_LIGHT_XYL_MSG_MINLEN_SET 7
#define BT_MESH_LIGHT_XYL_MSG_MAXLEN_SET 9

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_XYL_H__ */

/** @} */
