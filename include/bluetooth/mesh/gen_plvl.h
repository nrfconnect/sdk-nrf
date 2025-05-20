/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_plvl Generic Power Level Models
 * @{
 * @brief API for the Generic Power Level models.
 */

#ifndef BT_MESH_GEN_PLVL_H__
#define BT_MESH_GEN_PLVL_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Power Level set message parameters. */
struct bt_mesh_plvl_set {
	/** Power Level. */
	uint16_t power_lvl;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	const struct bt_mesh_model_transition *transition;
};

/** Power Level status message parameters. */
struct bt_mesh_plvl_status {
	/** Current Power Level. */
	uint16_t current;
	/** Target Power Level. */
	uint16_t target;
	/**
	 * Time remaining of the ongoing transition in milliseconds, or
	 * @em SYS_FOREVER_MS. If there's no ongoing transition,
	 * @c remaining_time is 0.
	 */
	int32_t remaining_time;
};

/** Power Level range parameters. */
struct bt_mesh_plvl_range {
	uint16_t min; /**< Minimum allowed Power Level. */
	uint16_t max; /**< Maximum allowed Power Level. */
};

/** Power Level range message parameters. */
struct bt_mesh_plvl_range_status {
	/** Status of the previous operation. */
	enum bt_mesh_model_status status;
	/** Current Power Level range. */
	struct bt_mesh_plvl_range range;
};

/**
 * @brief Convert Power Level to a percent.
 *
 * @param[in] plvl Raw Power Level.
 *
 * @return The Power Level in percent.
 */
static inline uint8_t bt_mesh_plvl_to_percent(uint16_t plvl)
{
	return (100UL * plvl) / UINT16_MAX;
}

/**
 * @brief Convert percent to raw Power Level.
 *
 * @param[in] plvl_percent Power Level in percent.
 *
 * @return The raw Power Level.
 */
static inline uint16_t bt_mesh_plvl_from_percent(uint8_t plvl_percent)
{
	return (UINT16_MAX * plvl_percent) / 100;
}

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_PLVL_OP_LEVEL_GET BT_MESH_MODEL_OP_2(0x82, 0x15)
#define BT_MESH_PLVL_OP_LEVEL_SET BT_MESH_MODEL_OP_2(0x82, 0x16)
#define BT_MESH_PLVL_OP_LEVEL_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x17)
#define BT_MESH_PLVL_OP_LEVEL_STATUS BT_MESH_MODEL_OP_2(0x82, 0x18)
#define BT_MESH_PLVL_OP_LAST_GET BT_MESH_MODEL_OP_2(0x82, 0x19)
#define BT_MESH_PLVL_OP_LAST_STATUS BT_MESH_MODEL_OP_2(0x82, 0x1A)
#define BT_MESH_PLVL_OP_DEFAULT_GET BT_MESH_MODEL_OP_2(0x82, 0x1B)
#define BT_MESH_PLVL_OP_DEFAULT_STATUS BT_MESH_MODEL_OP_2(0x82, 0x1C)
#define BT_MESH_PLVL_OP_RANGE_GET BT_MESH_MODEL_OP_2(0x82, 0x1D)
#define BT_MESH_PLVL_OP_RANGE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x1E)
#define BT_MESH_PLVL_OP_DEFAULT_SET BT_MESH_MODEL_OP_2(0x82, 0x1F)
#define BT_MESH_PLVL_OP_DEFAULT_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x20)
#define BT_MESH_PLVL_OP_RANGE_SET BT_MESH_MODEL_OP_2(0x82, 0x21)
#define BT_MESH_PLVL_OP_RANGE_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x22)

#define BT_MESH_PLVL_MSG_LEN_LEVEL_GET 0
#define BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET 3
#define BT_MESH_PLVL_MSG_MAXLEN_LEVEL_SET 5
#define BT_MESH_PLVL_MSG_MINLEN_LEVEL_STATUS 2
#define BT_MESH_PLVL_MSG_MAXLEN_LEVEL_STATUS 5
#define BT_MESH_PLVL_MSG_LEN_LAST_GET 0
#define BT_MESH_PLVL_MSG_LEN_LAST_STATUS 2
#define BT_MESH_PLVL_MSG_LEN_DEFAULT_GET 0
#define BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS 2
#define BT_MESH_PLVL_MSG_LEN_RANGE_GET 0
#define BT_MESH_PLVL_MSG_LEN_RANGE_STATUS 5
#define BT_MESH_PLVL_MSG_LEN_DEFAULT_SET 2
#define BT_MESH_PLVL_MSG_LEN_RANGE_SET 4
/** @endcond */


#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PLVL_H__ */

/** @} */
