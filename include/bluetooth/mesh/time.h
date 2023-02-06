/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @defgroup bt_mesh_time Time Models
 * @{
 * @brief API for the Time models.
 */

#ifndef BT_MESH_TIME_H__
#define BT_MESH_TIME_H__

#include <zephyr/types.h>
#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Time Role states */
enum bt_mesh_time_role {
	/** Does not participate in propagation of time information. */
	BT_MESH_TIME_NONE,
	/** Publishes Time Status messages but does not process received
	 *  Time Status messages.
	 */
	BT_MESH_TIME_AUTHORITY,
	/** Processes received and publishes Time Status messages. */
	BT_MESH_TIME_RELAY,
	/** Does not publish but processes received Time Status messages. */
	BT_MESH_TIME_CLIENT,
};

/** TAI time. */
struct bt_mesh_time_tai {
	uint64_t sec:40, /**< Seconds */
		 subsec:8; /**< 1/256th seconds */
};

/** Parameters for the Time Status message. */
struct bt_mesh_time_status {
	/** TAI time. */
	struct bt_mesh_time_tai tai;
	/** Accumulated uncertainty of the mesh Timestamp in milliseconds. */
	uint64_t uncertainty;
	/** Current TAI-UTC Delta (leap seconds). */
	int16_t tai_utc_delta;
	/** Current zone offset in 15-minute increments. */
	int16_t time_zone_offset;
	/** Reliable TAI source flag. */
	bool is_authority;
};

/** Time zone change. */
struct bt_mesh_time_zone_change {
	/** New zone offset in 15-minute increments. */
	int16_t new_offset;
	/** TAI update point for Time Zone Offset. */
	uint64_t timestamp;
};

/** Parameters for the Time Zone Status message. */
struct bt_mesh_time_zone_status {
	/** Current zone offset in 15-minute increments. */
	int16_t current_offset;
	/** New zone update context */
	struct bt_mesh_time_zone_change time_zone_change;
};

/** Time UTC Delta change. */
struct bt_mesh_time_tai_utc_change {
	/** New TAI-UTC Delta (leap seconds). */
	int16_t delta_new;
	/** TAI update point for TAI-UTC Delta. */
	uint64_t timestamp;
};

/** Parameters for the Time Zone UTC delta Status message. */
struct bt_mesh_time_tai_utc_delta_status {
	/** Current TAI-UTC Delta (leap seconds). */
	int16_t delta_current;
	/** New TAI-UTC update context */
	struct bt_mesh_time_tai_utc_change tai_utc_change;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_TIME_OP_TIME_GET BT_MESH_MODEL_OP_2(0x82, 0x37)
#define BT_MESH_TIME_OP_TIME_SET BT_MESH_MODEL_OP_1(0x5C)
#define BT_MESH_TIME_OP_TIME_STATUS BT_MESH_MODEL_OP_1(0x5D)
#define BT_MESH_TIME_OP_TIME_ROLE_GET BT_MESH_MODEL_OP_2(0x82, 0x38)
#define BT_MESH_TIME_OP_TIME_ROLE_SET BT_MESH_MODEL_OP_2(0x82, 0x39)
#define BT_MESH_TIME_OP_TIME_ROLE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x3A)
#define BT_MESH_TIME_OP_TIME_ZONE_GET BT_MESH_MODEL_OP_2(0x82, 0x3B)
#define BT_MESH_TIME_OP_TIME_ZONE_SET BT_MESH_MODEL_OP_2(0x82, 0x3C)
#define BT_MESH_TIME_OP_TIME_ZONE_STATUS BT_MESH_MODEL_OP_2(0x82, 0x3D)
#define BT_MESH_TIME_OP_TAI_UTC_DELTA_GET BT_MESH_MODEL_OP_2(0x82, 0x3E)
#define BT_MESH_TIME_OP_TAI_UTC_DELTA_SET BT_MESH_MODEL_OP_2(0x82, 0x3F)
#define BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS BT_MESH_MODEL_OP_2(0x82, 0x40)

#define BT_MESH_TIME_MSG_LEN_GET 0
#define BT_MESH_TIME_MSG_LEN_TIME_SET 10
#define BT_MESH_TIME_MSG_MINLEN_TIME_STATUS 5
#define BT_MESH_TIME_MSG_MAXLEN_TIME_STATUS 10
#define BT_MESH_TIME_MSG_LEN_TIME_ROLE_SET 1
#define BT_MESH_TIME_MSG_LEN_TIME_ROLE_STATUS 1
#define BT_MESH_TIME_MSG_LEN_TIME_ZONE_SET 6
#define BT_MESH_TIME_MSG_LEN_TIME_ZONE_STATUS 7
#define BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_SET 7
#define BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_STATUS 9

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TIME_H__ */

/** @} */
