/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_scheduler Scheduler models
 * @{
 * @brief Common API for the Scheduler models.
 */

#ifndef BT_MESH_SCHEDULER_H__
#define BT_MESH_SCHEDULER_H__

#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of entries of the Schedule Register state . */
#define BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT  16u

/** Scheduled at any year. */
#define BT_MESH_SCHEDULER_ANY_YEAR         0x64
/** Scheduled at any day of the month. */
#define BT_MESH_SCHEDULER_ANY_DAY          0x00
/** Scheduled at any hour of the day. */
#define BT_MESH_SCHEDULER_ANY_HOUR         0x18
/** Scheduled once a day at a random hour. */
#define BT_MESH_SCHEDULER_ONCE_A_DAY       0x19
/** Scheduled at any minute of the hour. */
#define BT_MESH_SCHEDULER_ANY_MINUTE       0x3C
/** Scheduled every 15 minutes (minute modulo 15 is 0) (0, 15, 30, 45). */
#define BT_MESH_SCHEDULER_EVERY_15_MINUTES 0x3D
/** Scheduled every 20 minutes (minute modulo 20 is 0) (0, 20, 40). */
#define BT_MESH_SCHEDULER_EVERY_20_MINUTES 0x3E
/** Scheduled once an hour (at a random minute). */
#define BT_MESH_SCHEDULER_ONCE_AN_HOUR     0x3F
/** Scheduled at any second of the minute. */
#define BT_MESH_SCHEDULER_ANY_SECOND       0x3C
/** Scheduled every 15 seconds (minute modulo 15 is 0) (0, 15, 30, 45). */
#define BT_MESH_SCHEDULER_EVERY_15_SECONDS 0x3D
/** Scheduled every 20 seconds (minute modulo 20 is 0) (0, 20, 40). */
#define BT_MESH_SCHEDULER_EVERY_20_SECONDS 0x3E
/** Scheduled once a minute (at a random second). */
#define BT_MESH_SCHEDULER_ONCE_A_MINUTE    0x3F

/** Representation of the months of the year
 * in which the scheduled event is enabled.
 */
enum bt_mesh_scheduler_month {
	BT_MESH_SCHEDULER_JAN = BIT(0),
	BT_MESH_SCHEDULER_FEB = BIT(1),
	BT_MESH_SCHEDULER_MAR = BIT(2),
	BT_MESH_SCHEDULER_APR = BIT(3),
	BT_MESH_SCHEDULER_MAY = BIT(4),
	BT_MESH_SCHEDULER_JUN = BIT(5),
	BT_MESH_SCHEDULER_JUL = BIT(6),
	BT_MESH_SCHEDULER_AUG = BIT(7),
	BT_MESH_SCHEDULER_SEP = BIT(8),
	BT_MESH_SCHEDULER_OCT = BIT(9),
	BT_MESH_SCHEDULER_NOV = BIT(10),
	BT_MESH_SCHEDULER_DEC = BIT(11)
};

/** Representation of the days of the week
 * in which the scheduled event is enabled.
 */
enum bt_mesh_scheduler_wday {
	BT_MESH_SCHEDULER_MON = BIT(0),
	BT_MESH_SCHEDULER_TUE = BIT(1),
	BT_MESH_SCHEDULER_WED = BIT(2),
	BT_MESH_SCHEDULER_THU = BIT(3),
	BT_MESH_SCHEDULER_FRI = BIT(4),
	BT_MESH_SCHEDULER_SAT = BIT(5),
	BT_MESH_SCHEDULER_SUN = BIT(6),
};

/** Representation of the actions to be executed for a scheduled event. */
enum bt_mesh_scheduler_action {
	BT_MESH_SCHEDULER_TURN_OFF,
	BT_MESH_SCHEDULER_TURN_ON,
	BT_MESH_SCHEDULER_SCENE_RECALL,
	BT_MESH_SCHEDULER_NO_ACTIONS = 0xf
};

/** Schedule Register entry. */
struct bt_mesh_schedule_entry {
	/** Two last digits of the scheduled year for the action, or
	 * @ref BT_MESH_SCHEDULER_ANY_YEAR.
	 */
	uint8_t year;
	/** Scheduled month for the action. */
	enum bt_mesh_scheduler_month month : 12;
	/** Scheduled day of the month for the action. */
	uint8_t day;
	/** Scheduled hour for the action. */
	uint8_t hour;
	/** Scheduled minute for the action. */
	uint8_t minute;
	/** Scheduled second for the action. */
	uint8_t second;
	/** Schedule days of the week for the action. */
	enum bt_mesh_scheduler_wday day_of_week : 7;
	/** Action to be performed at the scheduled time. */
	enum bt_mesh_scheduler_action action : 4;
	/** Transition time for this action. */
	uint8_t transition_time;
	/** Scene number to be used for some actions. */
	uint16_t scene_number;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_SCHEDULER_OP_GET              BT_MESH_MODEL_OP_2(0x82, 0x49)
#define BT_MESH_SCHEDULER_OP_STATUS           BT_MESH_MODEL_OP_2(0x82, 0x4A)
#define BT_MESH_SCHEDULER_OP_ACTION_GET       BT_MESH_MODEL_OP_2(0x82, 0x48)
#define BT_MESH_SCHEDULER_OP_ACTION_STATUS    BT_MESH_MODEL_OP_1(0x5F)
#define BT_MESH_SCHEDULER_OP_ACTION_SET       BT_MESH_MODEL_OP_1(0x60)
#define BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK BT_MESH_MODEL_OP_1(0x61)

#define BT_MESH_SCHEDULER_MSG_LEN_ACTION_GET 1
#define BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET 10
#define BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS 10
#define BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED 1
#define BT_MESH_SCHEDULER_MSG_LEN_GET 0
#define BT_MESH_SCHEDULER_MSG_LEN_STATUS 2

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SCHEDULER_H__ */

/** @} */
