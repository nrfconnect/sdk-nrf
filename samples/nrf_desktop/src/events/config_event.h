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

#define DATA_SIZE_USER_CONFIG 4

#if (REPORT_SIZE_USER_CONFIG <= DATA_SIZE_USER_CONFIG)
#error "Invalid size of report"
#endif

enum config_event_id {
	CONFIG_EVENT_ID_MOTION_CPI = 0,
	CONFIG_EVENT_ID_MOTION_DOWNSHIFT_RUN = 1,
	CONFIG_EVENT_ID_MOTION_DOWNSHIFT_REST1 = 2,
	CONFIG_EVENT_ID_MOTION_DOWNSHIFT_REST2 = 3,

	CONFIG_EVENT_ID_MAX
};

/** @brief Configuration channel event.
 * Used to change firmware parameters at runtime.
 */
struct config_event {
	struct event_header header;

	enum config_event_id id;
	u8_t data[DATA_SIZE_USER_CONFIG];
	bool store_needed;
};

EVENT_TYPE_DECLARE(config_event);


/** @brief Configuration channel forward event.
 * Used to pass configuration from dongle to connected devices.
 */
struct config_forward_event {
	struct event_header header;

	u16_t recipient;
	enum config_event_id id;
	u8_t data[DATA_SIZE_USER_CONFIG];
};

EVENT_TYPE_DECLARE(config_forward_event);


enum forward_status {
	FORWARD_STATUS_PENDING,
	FORWARD_STATUS_SUCCESS,
	FORWARD_STATUS_WRITE_ERROR,
	FORWARD_STATUS_DISCONNECTED_ERROR,
};

/** @brief Configuration channel forwarded event.
 * Used to confirm that event has been successfully forwarded.
 */
struct config_forwarded_event {
	struct event_header header;

	enum forward_status status;
};

EVENT_TYPE_DECLARE(config_forwarded_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CONFIG_EVENT_H_ */
