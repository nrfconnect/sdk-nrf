/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief USB event header file.
 */

#ifndef _USB_EVENT_H_
#define _USB_EVENT_H_

#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB Event
 * @defgroup nrf_desktop_usb_event USB Event
 *
 * File defines a set of events used to transmit the information about USB
 * state between the application modules.
 *
 * @{
 */

/** @brief Peer states. */
enum usb_state {
	/** Cable is not attached. */
	USB_STATE_DISCONNECTED,
	/** Cable attached but no communication. */
	USB_STATE_POWERED,
	/** Cable attached and data is exchanged. */
	USB_STATE_ACTIVE,
	/** Cable attached but port is suspended. */
	USB_STATE_SUSPENDED,

	/** Number of states. */
	USB_STATE_COUNT
};

/** @brief USB state event. */
struct usb_state_event {
	/** Event header. */
	struct app_event_header header;

	/** State of the USB module. */
	enum usb_state state;
};
APP_EVENT_TYPE_DECLARE(usb_state_event);

/** @brief USB HID event. */
struct usb_hid_event {
	/** Event header. */
	struct app_event_header header;

	/** USB HID device id. */
	const void *id;
	/** USB HID device enabled state. */
	bool enabled;
};
APP_EVENT_TYPE_DECLARE(usb_hid_event);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _USB_EVENT_H_ */
