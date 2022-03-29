/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HIDS_EVENT_H_
#define _HIDS_EVENT_H_

#include <bluetooth/services/hids.h>

#include <app_evt_mgr.h>
#include <app_evt_mgr_profiler_tracer.h>
#include "profiler.h"
#include "hid_report_desc.h"


/**
 * @brief HID Service Events
 * @defgroup hids_event HID Service Events
 *
 * File defines a set of events used by HID service.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HID notification event. */
struct hid_notification_event {
	struct application_event_header header; /**< Event header. */

	uint8_t report_id; /**< HID report id. */
	bool enabled; /**< True if report is enabled. */
};

APPLICATION_EVENT_TYPE_DECLARE(hid_notification_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HIDS_EVENT_H_ */
