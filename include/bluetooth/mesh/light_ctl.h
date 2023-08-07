/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @defgroup bt_mesh_light_ctl Light CTL models
 * @{
 * @brief API for the Light CTL models.
 */

#ifndef BT_MESH_LIGHT_CTL_H__
#define BT_MESH_LIGHT_CTL_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Minimum permitted Temperature level */
#define BT_MESH_LIGHT_TEMP_MIN 800
/** Maximum permitted Temperature level */
#define BT_MESH_LIGHT_TEMP_MAX 20000
/** Unknown Temperature range boundaries */
#define BT_MESH_LIGHT_TEMP_UNKNOWN 0xFFFF

/** All Light CTL parameters */
struct bt_mesh_light_ctl {
	/** Light level */
	uint16_t light;
	/** Light temperature */
	uint16_t temp;
	/** Delta UV */
	int16_t delta_uv;
};

/** Light temperature parameters. */
struct bt_mesh_light_temp {
	/** Light temperature */
	uint16_t temp;
	/** Delta UV */
	int16_t delta_uv;
};

/** Light CTL set message parameters. */
struct bt_mesh_light_ctl_set {
	/** CTL set parameters. */
	struct bt_mesh_light_ctl params;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Light CTL status message parameters. */
struct bt_mesh_light_ctl_status {
	/** Current light level. */
	uint16_t current_light;
	/** Current light temperature. */
	uint16_t current_temp;
	/** Target light level. */
	uint16_t target_light;
	/** Target light temperature. */
	uint16_t target_temp;
	/** Remaining time for the state change (ms). */
	uint32_t remaining_time;
};

/** Light CTL Temperature set message parameters. */
struct bt_mesh_light_temp_set {
	/** New light temperature. */
	struct bt_mesh_light_temp params;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Light temperature status. */
struct bt_mesh_light_temp_status {
	/** Current light temperature parameters. */
	struct bt_mesh_light_temp current;
	/** Target light temperature parameters. */
	struct bt_mesh_light_temp target;
	/** Remaining time for the state change (ms). */
	uint32_t remaining_time;
};

/** Light temperature range parameters. */
struct bt_mesh_light_temp_range {
	/** Minimum light temperature. */
	uint16_t min;
	/** Maximum light temperature. */
	uint16_t max;
};

/** Light CTL range status message parameters. */
struct bt_mesh_light_temp_range_status {
	/** Status of the previous operation. */
	enum bt_mesh_model_status status;
	/** Current light temperature range. */
	struct bt_mesh_light_temp_range range;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LIGHT_CTL_GET BT_MESH_MODEL_OP_2(0x82, 0x5D)
#define BT_MESH_LIGHT_CTL_SET BT_MESH_MODEL_OP_2(0x82, 0x5E)
#define BT_MESH_LIGHT_CTL_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x5F)
#define BT_MESH_LIGHT_CTL_STATUS BT_MESH_MODEL_OP_2(0x82, 0x60)
#define BT_MESH_LIGHT_TEMP_GET BT_MESH_MODEL_OP_2(0x82, 0x61)
#define BT_MESH_LIGHT_TEMP_RANGE_GET BT_MESH_MODEL_OP_2(0x82, 0x62)
#define BT_MESH_LIGHT_TEMP_RANGE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x63)
#define BT_MESH_LIGHT_TEMP_SET BT_MESH_MODEL_OP_2(0x82, 0x64)
#define BT_MESH_LIGHT_TEMP_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x65)
#define BT_MESH_LIGHT_TEMP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x66)
#define BT_MESH_LIGHT_CTL_DEFAULT_GET BT_MESH_MODEL_OP_2(0x82, 0x67)
#define BT_MESH_LIGHT_CTL_DEFAULT_STATUS BT_MESH_MODEL_OP_2(0x82, 0x68)
#define BT_MESH_LIGHT_CTL_DEFAULT_SET BT_MESH_MODEL_OP_2(0x82, 0x69)
#define BT_MESH_LIGHT_CTL_DEFAULT_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x6A)
#define BT_MESH_LIGHT_TEMP_RANGE_SET BT_MESH_MODEL_OP_2(0x82, 0x6B)
#define BT_MESH_LIGHT_TEMP_RANGE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x6C)

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
