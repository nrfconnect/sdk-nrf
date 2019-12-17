/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_loc Generic Location models
 * @{
 * @brief API for the Generic Location models.
 */

#ifndef BT_MESH_GEN_LOC_H__
#define BT_MESH_GEN_LOC_H__

#include <bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_LOC_OP_GLOBAL_GET BT_MESH_MODEL_OP_2(0x82, 0x25)
#define BT_MESH_LOC_OP_GLOBAL_STATUS BT_MESH_MODEL_OP_1(0x40)
#define BT_MESH_LOC_OP_GLOBAL_SET BT_MESH_MODEL_OP_1(0x41)
#define BT_MESH_LOC_OP_GLOBAL_SET_UNACK BT_MESH_MODEL_OP_1(0x42)
#define BT_MESH_LOC_OP_LOCAL_GET BT_MESH_MODEL_OP_2(0x82, 0x26)
#define BT_MESH_LOC_OP_LOCAL_STATUS BT_MESH_MODEL_OP_2(0x82, 0x27)
#define BT_MESH_LOC_OP_LOCAL_SET BT_MESH_MODEL_OP_2(0x82, 0x28)
#define BT_MESH_LOC_OP_LOCAL_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x29)

#define BT_MESH_LOC_MSG_LEN_GLOBAL_GET 0
#define BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS 10
#define BT_MESH_LOC_MSG_LEN_GLOBAL_SET 10
#define BT_MESH_LOC_MSG_LEN_LOCAL_GET 0
#define BT_MESH_LOC_MSG_LEN_LOCAL_STATUS 9
#define BT_MESH_LOC_MSG_LEN_LOCAL_SET 9
/** @endcond */

/**
 * @defgroup bt_mesh_loc_altitude_defines Defines for altitude.
 * Boundaries and special values for the altitude.
 * @{
 */
/** Highest altitude that can be represented */
#define BT_MESH_LOC_ALTITUDE_MAX 32765
/** Altitude is unknown or not configured. */
#define BT_MESH_LOC_ALTITUDE_UNKNOWN 0x7fff
/** Altitude is out of bounds. */
#define BT_MESH_LOC_ALTITUDE_TOO_LARGE 0x7ffe
/** @} */

/**
 * @defgroup bt_mesh_loc_floor_number_defines Defines for floor number.
 * Boundaries and special values for the local floor number.
 * @{
 */
/** Lowest floor number that can be represented. */
#define BT_MESH_LOC_FLOOR_NUMBER_MIN (-20)
/** Highest floor number that can be represented. */
#define BT_MESH_LOC_FLOOR_NUMBER_MAX 232
/** Ground floor (where the ground floor is referred to as floor 0) */
#define BT_MESH_LOC_FLOOR_NUMBER_GROUND_FLOOR_0 0xfc
/** Ground floor (where the ground floor is referred to as floor 1) */
#define BT_MESH_LOC_FLOOR_NUMBER_GROUND_FLOOR_1 0xfd
/** Unknown or unconfigured floor number. */
#define BT_MESH_LOC_FLOOR_NUMBER_UNKNOWN 0xff
/** @} */

/** Global location parameters. */
struct bt_mesh_loc_global {
	/** Global WGS84 North coordinate in degrees. */
	double latitude;
	/** Global WGS84 East coordinate in degrees. */
	double longitude;
	/**
	 * Global altitude above the WGS84 datum in meters.
	 * @sa bt_mesh_loc_altitude_defines
	 */
	s16_t altitude;
};

/** Local location parameters. */
struct bt_mesh_loc_local {
	/** Local north position in decimeters. */
	s16_t north;
	/** Local east position in decimeters. */
	s16_t east;
	/** Local altitude in decimeters. @sa bt_mesh_loc_altitude_defines */
	s16_t altitude;
	/** Floor number. @sa bt_mesh_loc_floor_number_defines */
	s16_t floor_number;
	/** Whether the device is movable. */
	bool is_mobile;
	/** Time since the previous position update, or @ref K_FOREVER. */
	s32_t time_delta;
	/** Precision of the location in millimeters. */
	u32_t precision_mm;
};

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LOC_H__ */

/** @} */
