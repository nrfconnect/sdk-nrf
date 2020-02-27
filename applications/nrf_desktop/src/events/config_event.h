/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _CONFIG_EVENT_H_
#define _CONFIG_EVENT_H_

/**
 * @brief Config Event
 * @defgroup config_event Config Event
 * @{
 */

#include "event_manager.h"
#include "hid_report_desc.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Config event ID macros */
#define GROUP_FIELD_POS		6
#define GROUP_FIELD_SIZE	2
#define GROUP_FIELD_MASK	BIT_MASK(GROUP_FIELD_SIZE)
#define GROUP_FIELD_SET(group)	((group & GROUP_FIELD_MASK) << GROUP_FIELD_POS)
#define GROUP_FIELD_GET(event_id) ((event_id >> GROUP_FIELD_POS) & GROUP_FIELD_MASK)

#define TYPE_FIELD_POS		0
#define TYPE_FIELD_SIZE		6
#define TYPE_FIELD_MASK		BIT_MASK(TYPE_FIELD_SIZE)
#define TYPE_FIELD_SET(type)	((type & TYPE_FIELD_MASK) << TYPE_FIELD_POS)
#define TYPE_FIELD_GET(event_id) ((event_id >> TYPE_FIELD_POS) & TYPE_FIELD_MASK)

#define CONFIG_EVENT_ID(group, type) ((u8_t)(GROUP_FIELD_SET(group) | \
					     TYPE_FIELD_SET(type)))

#define EVENT_GROUP_SETUP	0x1
#define EVENT_GROUP_DFU		0x2
#define EVENT_GROUP_LED_STREAM	0x3


/* Config event, setup group macros */

#define MOD_FIELD_POS		3
#define MOD_FIELD_SIZE		3
#define MOD_FIELD_MASK		BIT_MASK(MOD_FIELD_SIZE)
#define MOD_FIELD_SET(module)	((module & MOD_FIELD_MASK) << MOD_FIELD_POS)
#define MOD_FIELD_GET(event_id) ((event_id >> MOD_FIELD_POS) & MOD_FIELD_MASK)

#define OPT_FIELD_POS		0
#define OPT_FIELD_SIZE		3
#define OPT_FIELD_MASK		BIT_MASK(OPT_FIELD_SIZE)
#define OPT_FIELD_SET(option)	((option & OPT_FIELD_MASK) << OPT_FIELD_POS)
#define OPT_FIELD_GET(event_id) ((event_id >> OPT_FIELD_POS) & OPT_FIELD_MASK)

#define SETUP_EVENT_ID(module, option) CONFIG_EVENT_ID(EVENT_GROUP_SETUP, \
						       MOD_FIELD_SET(module) | OPT_FIELD_SET(option))

#define SETUP_MODULE_SENSOR	0x1
#define SETUP_MODULE_QOS	0x2
#define SETUP_MODULE_BLE_BOND	0x3


/* Config event, setup group, sensor module macros */
#define SENSOR_OPT_CPI			0x0
#define SENSOR_OPT_DOWNSHIFT_RUN	0x1
#define SENSOR_OPT_DOWNSHIFT_REST1	0x2
#define SENSOR_OPT_DOWNSHIFT_REST2	0x3
#define SENSOR_OPT_COUNT 4

/* Config event, setup group, qos module macros */
#define QOS_OPT_BLACKLIST	0x0
#define QOS_OPT_CHMAP		0x1
#define QOS_OPT_PARAM_BLE	0x2
#define QOS_OPT_PARAM_WIFI	0x3
#define QOS_OPT_COUNT		4

/* Config event, setup group, ble_bond module macros */
#define BLE_BOND_PEER_ERASE	0x0
#define BLE_BOND_PEER_SEARCH	0x1
#define BLE_BOND_COUNT		2

/* Config event, DFU group macros */
#define DFU_START	0x0
#define DFU_DATA	0x1
#define DFU_SYNC	0x2
#define DFU_REBOOT	0x3
#define DFU_IMGINFO	0x4

/* Config event, led stream group macros */
#define LED_STREAM_DATA 0x0

/** @brief Configuration channel event.
 * Used to change firmware parameters at runtime.
 */
struct config_event {
	struct event_header header;

	u8_t id;
	struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(config_event);

/** @brief Configuration channel fetch event.
 * Used to fetch firmware parameters to host.
 */
struct config_fetch_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	void *channel_id;
	struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(config_fetch_event);

/** @brief Configuration channel fetch request event.
 * Used to request fetching firmware parameters to host.
 */
struct config_fetch_request_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	void *channel_id;
};

EVENT_TYPE_DECLARE(config_fetch_request_event);

enum config_status {
	CONFIG_STATUS_SUCCESS,
	CONFIG_STATUS_PENDING,
	CONFIG_STATUS_FETCH,
	CONFIG_STATUS_TIMEOUT,
	CONFIG_STATUS_REJECT,
	CONFIG_STATUS_WRITE_ERROR,
	CONFIG_STATUS_DISCONNECTED_ERROR,
};

/** @brief Configuration channel forward event.
 * Used to pass configuration from dongle to connected devices.
 */
struct config_forward_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	enum config_status status;

	struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(config_forward_event);

/** @brief Configuration channel forward get event.
 * Used to forward configuration channel get request to connected devices.
 */
struct config_forward_get_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	void *channel_id;
	enum config_status status;
};

EVENT_TYPE_DECLARE(config_forward_get_event);

/** @brief Configuration channel forwarded event.
 * Used to confirm that event has been successfully forwarded.
 */
struct config_forwarded_event {
	struct event_header header;

	enum config_status status;
};

EVENT_TYPE_DECLARE(config_forwarded_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CONFIG_EVENT_H_ */
