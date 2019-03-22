/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _USB_EVENT_H_
#define _USB_EVENT_H_

/**
 * @brief USB Event
 * @defgroup usb_event USB Event
 *
 * File defines a set of events used to transmit the information about USB
 * state between the application modules.
 *
 * @{
 */

#include <toolchain/common.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Peer states. */
enum usb_state {
	USB_STATE_DISCONNECTED, /**< Cable is not attached. */
	USB_STATE_POWERED,      /**< Cable attached but no communication. */
	USB_STATE_IDLE,		/**< Cable attached but no communication. */
	USB_STATE_ACTIVE,       /**< Cable attached and data is exchanged. */
	USB_STATE_SUSPENDED,    /**< Cable attached but port is suspended. */

	USB_STATE_COUNT         /**< Number of states. */
};

/** @brief USB state event. */
struct usb_state_event {
	struct event_header header; /**< Event header. */

	enum usb_state state; /**< State of the USB module. */
	const void *id;       /**< Module id. */
};
EVENT_TYPE_DECLARE(usb_state_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _USB_EVENT_H_ */
