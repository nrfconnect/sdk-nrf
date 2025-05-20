/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_battery Generic Battery models
 * @{
 * @brief API for the Generic Battery models.
 */

#ifndef BT_MESH_GEN_BATTERY_H__
#define BT_MESH_GEN_BATTERY_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Unknown battery level. */
#define BT_MESH_BATTERY_LVL_UNKNOWN 0xff

/** Longest acceptable charge and discharge time in minutes */
#define BT_MESH_BATTERY_TIME_MAX 0xfffffe
/** Unknown discharge time */
#define BT_MESH_BATTERY_TIME_UNKNOWN 0xffffff

/** Battery presence state. */
enum bt_mesh_battery_presence {
	/** The battery is not present. */
	BT_MESH_BATTERY_PRESENCE_NOT_PRESENT,
	/** The battery is present and is removable. */
	BT_MESH_BATTERY_PRESENCE_PRESENT_REMOVABLE,
	/** The battery is present and is non-removable. */
	BT_MESH_BATTERY_PRESENCE_PRESENT_NOT_REMOVABLE,
	/** The battery presence is unknown. */
	BT_MESH_BATTERY_PRESENCE_UNKNOWN,
};

/** Indicator for the charge level of the battery. */
enum bt_mesh_battery_indicator {
	/** The battery charge is Critically Low Level. */
	BT_MESH_BATTERY_INDICATOR_CRITICALLY_LOW,
	/** The battery charge is Low Level. */
	BT_MESH_BATTERY_INDICATOR_LOW,
	/** The battery charge is Good Level. */
	BT_MESH_BATTERY_INDICATOR_GOOD,
	/** The battery charge is unknown. */
	BT_MESH_BATTERY_INDICATOR_UNKNOWN,
};

/** Battery charging state. */
enum bt_mesh_battery_charging {
	/** The battery is not chargeable. */
	BT_MESH_BATTERY_CHARGING_NOT_CHARGEABLE,
	/** The battery is chargeable and is not charging. */
	BT_MESH_BATTERY_CHARGING_CHARGEABLE_NOT_CHARGING,
	/** The battery is chargeable and is charging. */
	BT_MESH_BATTERY_CHARGING_CHARGEABLE_CHARGING,
	/** The battery charging state is unknown. */
	BT_MESH_BATTERY_CHARGING_UNKNOWN,
};

/** Battery service state. */
enum bt_mesh_battery_service {
	/** Reserved for future use. */
	BT_MESH_BATTERY_SERVICE_INVALID,
	/** The battery does not require service. */
	BT_MESH_BATTERY_SERVICE_NOT_REQUIRED,
	/** The battery requires service. */
	BT_MESH_BATTERY_SERVICE_REQUIRED,
	/** The battery serviceability is unknown. */
	BT_MESH_BATTERY_SERVICE_UNKNOWN,
};

struct bt_mesh_battery_status {
	/**
	 * Current battery level in percent, or
	 * @ref BT_MESH_BATTERY_LVL_UNKNOWN.
	 */
	uint8_t battery_lvl;
	/** Minutes until discharged, or @ref BT_MESH_BATTERY_TIME_UNKNOWN. */
	uint32_t discharge_minutes;
	/** Minutes until discharged, or @ref BT_MESH_BATTERY_TIME_UNKNOWN. */
	uint32_t charge_minutes;
	/** Presence state. */
	enum bt_mesh_battery_presence presence;
	/** Charge level indicator. */
	enum bt_mesh_battery_indicator indicator;
	/** Charging state. */
	enum bt_mesh_battery_charging charging;
	/** Service state. */
	enum bt_mesh_battery_service service;
};

/** @cond INTERNAL_HIDDEN */
#define BT_MESH_BATTERY_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x23)
#define BT_MESH_BATTERY_OP_STATUS BT_MESH_MODEL_OP_2(0x82, 0x24)

#define BT_MESH_BATTERY_MSG_LEN_GET 0
#define BT_MESH_BATTERY_MSG_LEN_STATUS 8
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_BATTERY_H__ */

/** @} */
