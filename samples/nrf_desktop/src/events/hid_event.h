/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _HID_EVENT_H_
#define _HID_EVENT_H_

/**
 * @brief HID Events
 * @defgroup hid_event HID Events
 *
 * File defines a set of events used to transmit the HID report data between
 * application modules.
 *
 * @{
 */

#include "event_manager.h"
#include "profiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Keyboard report data event. */
struct hid_keyboard_event {
	struct event_header header; /**< Event header. */

	u8_t modifier_bm; /**< Bitmask indicating pressed modifier keys. */
	u8_t keys[6];     /**< Array of pressed keys' usage values. */
};

EVENT_TYPE_DECLARE(hid_keyboard_event);


/** @brief Mouse cursor position change report data event. */
struct hid_mouse_xy_event {
	struct event_header header; /**< Event header. */

	s16_t dx; /**< Position change in x axis. */
	s16_t dy; /**< Position change in y axis. */
};

EVENT_TYPE_DECLARE(hid_mouse_xy_event);


/** @brief Mouse wheel/pan report data event. */
struct hid_mouse_wp_event {
	struct event_header header; /**< Event header. */

	s16_t wheel; /**< Change of wheel (scroll). */
	s16_t pan;   /**< Change of pan. */
};

EVENT_TYPE_DECLARE(hid_mouse_wp_event);


/** @brief Mouse buttons report data event. */
struct hid_mouse_button_event {
	struct event_header header; /**< Event header. */

	u8_t button_bm; /**< Bitmask indicating pressed mouse buttons. */
};

EVENT_TYPE_DECLARE(hid_mouse_button_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HID_EVENT_H_ */
