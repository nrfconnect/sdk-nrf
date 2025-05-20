/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_loc Generic Location models
 * @{
 * @brief API for the Generic Location models.
 */

#ifndef BT_MESH_GEN_LOC_H__
#define BT_MESH_GEN_LOC_H__

#include <float.h>
#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_loc_location_defines Defines for locations.
 * Boundaries and special values for the latitude, longitude, altitude and local North/East.
 * @{
 */
/** The Global Longitude is unknown or not configured. */
#define BT_MESH_LOC_GLOBAL_LONGITUDE_UNKNOWN DBL_MAX
/** The Global Latitude is unknown or not configured. */
#define BT_MESH_LOC_GLOBAL_LATITUDE_UNKNOWN  DBL_MAX
/** The Local North is unknown or not configured. */
#define BT_MESH_LOC_LOCAL_NORTH_UNKNOWN      ((int16_t)0x8000)
/** The Local East is unknown or not configured. */
#define BT_MESH_LOC_LOCAL_EAST_UNKNOWN       ((int16_t)0x8000)
/** The highest Altitude (Local and Global) that can be represented */
#define BT_MESH_LOC_ALTITUDE_MAX             ((int16_t)32765)
/** The Altitude (Local and Global) is unknown or not configured. */
#define BT_MESH_LOC_ALTITUDE_UNKNOWN         ((int16_t)0x7fff)
/** The Altitude (Local and Global) is out of bounds. */
#define BT_MESH_LOC_ALTITUDE_TOO_LARGE       ((int16_t)0x7ffe)
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

/** Global location parameters. @sa bt_mesh_loc_location_defines */
struct bt_mesh_loc_global {
	/** Global WGS84 North coordinate in degrees. */
	double latitude;
	/** Global WGS84 East coordinate in degrees. */
	double longitude;
	/** Global altitude above the WGS84 datum in meters. */
	int16_t altitude;
};

/** Local location parameters. */
struct bt_mesh_loc_local {
	/** The Local North position in decimeters. @sa bt_mesh_loc_location_defines */
	int16_t north;
	/** The Local East position in decimeters. @sa bt_mesh_loc_location_defines */
	int16_t east;
	/** The Local Altitude in decimeters. @sa bt_mesh_loc_location_defines */
	int16_t altitude;
	/** Floor number. @sa bt_mesh_loc_floor_number_defines */
	int16_t floor_number;
	/** Whether the device is movable. */
	bool is_mobile;
	/** Time since the previous position update in milliseconds, or
	 *  @em SYS_FOREVER_MS.
	 */
	int32_t time_delta;
	/** Precision of the location in millimeters. */
	uint32_t precision_mm;
};

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

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LOC_H__ */

/** @} */
