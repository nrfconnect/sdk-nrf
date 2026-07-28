/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HOGP_EVENT_H_
#define _HOGP_EVENT_H_

#include <bluetooth/services/hids.h>
#include <zephyr/bluetooth/conn.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

/**
 * @brief HID Forward Events
 * @defgroup hogp_event HID Forward Events
 *
 * File defines a set of events used by the HID forward module.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HOGP SCI mode changed event. */
struct hogp_sci_mode_changed_event {
	struct app_event_header header; /**< Event header. */

	struct bt_conn *conn; /**< Bluetooth connection to the HID peripheral. */
	enum bt_hids_sci_mode_value mode; /**< Current HID SCI mode. */
};

APP_EVENT_TYPE_DECLARE(hogp_sci_mode_changed_event);

/** @brief HOGP SCI mode request event. */
struct hogp_sci_mode_req_event {
	struct app_event_header header; /**< Event header. */

	struct bt_conn *conn; /**< Bluetooth connection to the HID peripheral. */
	enum bt_hids_sci_mode_value mode; /**< Requested HID SCI mode. */
};

APP_EVENT_TYPE_DECLARE(hogp_sci_mode_req_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HOGP_EVENT_H_ */
