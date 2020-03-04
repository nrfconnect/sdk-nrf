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


/** @brief HID report event. */
struct hid_report_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	struct event_dyndata dyndata; /**< Report data. The first byte is a report id. */
};

EVENT_TYPE_DYNDATA_DECLARE(hid_report_event);


/** @brief Report subscriber event. */
struct hid_report_subscriber_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	bool connected; /**< True if subscriber is connected to the system. */
};

EVENT_TYPE_DECLARE(hid_report_subscriber_event);


/** @brief Report sent event. */
struct hid_report_sent_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	u8_t report_id; /**< Report id. */
	bool error; /**< If true error occured on send. */
};

EVENT_TYPE_DECLARE(hid_report_sent_event);


/** @brief Report subscription event. */
struct hid_report_subscription_event {
	struct event_header header; /**< Event header. */

	const void *subscriber; /**< Id of the report subscriber. */
	u8_t report_id; /**< Report id. */
	bool enabled; /**< True if notification are enabled. */
};

EVENT_TYPE_DECLARE(hid_report_subscription_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HID_EVENT_H_ */
