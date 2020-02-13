/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _HID_EVENT_H_
#define _HID_EVENT_H_

#include "event_manager.h"
#include "profiler.h"
#include "hid_report_desc.h"


/**
 * @brief HID Events
 * @defgroup hid_event HID Events
 *
 * File defines a set of events used to transmit the HID report data between
 * application modules.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Keyboard report data event. */
struct hid_keyboard_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	u8_t modifier_bm; /**< Bitmask indicating pressed modifier keys. */
	u8_t keys[6];     /**< Array of pressed keys' usage values. */
};

EVENT_TYPE_DECLARE(hid_keyboard_event);


/** @brief Mouse report data event. */
struct hid_mouse_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	u8_t  button_bm;  /**< Bitmask indicating pressed mouse buttons. */
	s16_t wheel;      /**< Change of wheel (scroll). */
	s16_t dx;         /**< Position change in x axis. */
	s16_t dy;         /**< Position change in y axis. */
};

EVENT_TYPE_DECLARE(hid_mouse_event);


/** @brief System/consumer control report data event. */
struct hid_ctrl_event {
	struct event_header header; /**< Event header. */

	enum in_report report_type; /**< Type of HID control (system/consumer). */
	const void *subscriber; /**< Id of the report subscriber. */
	u16_t usage;            /**< Usage of CC button pressed. */
};

EVENT_TYPE_DECLARE(hid_ctrl_event);


/** @brief Report subscriber event. */
struct hid_report_subscriber_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	bool connected;   /**< True if subscriber is connected to the system. */
};

EVENT_TYPE_DECLARE(hid_report_subscriber_event);


/** @brief Report sent event. */
struct hid_report_sent_event {
	struct event_header header; /**< Event header. */

	const void *subscriber;     /**< Id of the report subscriber. */
	enum in_report report_type; /**< Type of the report. */
	bool error;                 /**< If true error occured on send. */
};

EVENT_TYPE_DECLARE(hid_report_sent_event);


/** @brief Report subscription event. */
struct hid_report_subscription_event {
	struct event_header header; /**< Event header. */

	const void *subscriber;     /**< Id of the report subscriber. */
	enum in_report report_type; /**< Type of the report. */
	bool enabled;               /**< True if notification are enabled. */
};

EVENT_TYPE_DECLARE(hid_report_subscription_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HID_EVENT_H_ */
