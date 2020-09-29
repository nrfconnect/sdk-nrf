/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @defgroup bt_mesh_light_ctl Light CTL models
 * @{
 * @brief API for the Light CTL models.
 */

#ifndef BT_MESH_LIGHT_CTL_H__
#define BT_MESH_LIGHT_CTL_H__

#include <bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Minimum permitted Temperature level */
#define BT_MESH_LIGHT_TEMP_RANGE_MIN 800
/** Maximum permitted Temperature level */
#define BT_MESH_LIGHT_TEMP_RANGE_MAX 20000

/** All Light CTL parameters */
struct bt_mesh_light_ctl {
	/** Light level */
	uint16_t light;
	/** Temperature level */
	uint16_t temp;
	/** Delta UV level */
	int16_t delta_uv;
};

/** Temperature subset of Light CTL parameters. */
struct bt_mesh_light_temp {
	/** Temperature level */
	uint16_t temp;
	/** Delta UV level */
	int16_t delta_uv;
};

/** Light CTL set message parameters. */
struct bt_mesh_light_ctl_set {
	/** CTL set parameters */
	struct bt_mesh_light_ctl params;
	/** Transition time parameters for the state change. */
	const struct bt_mesh_model_transition *transition;
};

/** Light CTL status message parameters. */
struct bt_mesh_light_ctl_status {
	/** Current light level */
	uint16_t current_light;
	/** Current Temperature level */
	uint16_t current_temp;
	/** Target light level */
	uint16_t target_light;
	/** Target Temperature level */
	uint16_t target_temp;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Light CTL Temperature set message parameters. */
struct bt_mesh_light_temp_set {
	/** CTL Temperature set parameters */
	struct bt_mesh_light_temp params;
	/** Transition time parameters for the state change. */
	const struct bt_mesh_model_transition *transition;
};

/** Light CTL Temperature status message parameters. */
struct bt_mesh_light_temp_status {
	/** Current Temperature set parameters */
	struct bt_mesh_light_temp current;
	/** Target Temperature set parameters */
	struct bt_mesh_light_temp target;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Light CTL range parameters. */
struct bt_mesh_light_temp_range {
	/** Minimum range value */
	uint16_t min;
	/** Maximum range value */
	uint16_t max;
};

/** Light CTL range status message parameters. */
struct bt_mesh_light_temp_range_status {
	/** Range set status code */
	enum bt_mesh_model_status status_code;
	/** Range set parameters */
	struct bt_mesh_light_temp_range range;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LIGHT_CTL_GET BT_MESH_MODEL_OP_2(0x82, 0X5D)
#define BT_MESH_LIGHT_CTL_SET BT_MESH_MODEL_OP_2(0x82, 0X5E)
#define BT_MESH_LIGHT_CTL_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0X5F)
#define BT_MESH_LIGHT_CTL_STATUS BT_MESH_MODEL_OP_2(0x82, 0X60)
#define BT_MESH_LIGHT_TEMP_GET BT_MESH_MODEL_OP_2(0x82, 0X61)
#define BT_MESH_LIGHT_TEMP_RANGE_GET BT_MESH_MODEL_OP_2(0x82, 0X62)
#define BT_MESH_LIGHT_TEMP_RANGE_STATUS BT_MESH_MODEL_OP_2(0x82, 0X63)
#define BT_MESH_LIGHT_TEMP_SET BT_MESH_MODEL_OP_2(0x82, 0X64)
#define BT_MESH_LIGHT_TEMP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0X65)
#define BT_MESH_LIGHT_TEMP_STATUS BT_MESH_MODEL_OP_2(0x82, 0X66)
#define BT_MESH_LIGHT_CTL_DEFAULT_GET BT_MESH_MODEL_OP_2(0x82, 0X67)
#define BT_MESH_LIGHT_CTL_DEFAULT_STATUS BT_MESH_MODEL_OP_2(0x82, 0X68)
#define BT_MESH_LIGHT_CTL_DEFAULT_SET BT_MESH_MODEL_OP_2(0x82, 0X69)
#define BT_MESH_LIGHT_CTL_DEFAULT_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0X6A)
#define BT_MESH_LIGHT_TEMP_RANGE_SET BT_MESH_MODEL_OP_2(0x82, 0X6B)
#define BT_MESH_LIGHT_TEMP_RANGE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0X6C)

#define BT_MESH_LIGHT_CTL_MSG_LEN_GET 0
#define BT_MESH_LIGHT_CTL_MSG_MINLEN_SET 7
#define BT_MESH_LIGHT_CTL_MSG_MAXLEN_SET 9
#define BT_MESH_LIGHT_CTL_MSG_MINLEN_STATUS 4
#define BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS 9
#define BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_STATUS 5
#define BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET 5
#define BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_SET 7
#define BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_STATUS 4
#define BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS 9
#define BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG 6
#define BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET 4
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTL_H__ */

/** @} */
