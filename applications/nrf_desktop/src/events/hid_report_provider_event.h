/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_REPORT_PROVIDER_EVENT_H_
#define _HID_REPORT_PROVIDER_EVENT_H_

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <nrf_profiler.h>


/**
 * @brief HID Report Provider Events
 * @defgroup hid_report_provider_event HID Report Provider Events
 *
 * File defines a set of events used to link HID report provider and HID state.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Structure describing HID subscriber connection state. */
struct subscriber_conn_state {
	const void *subscriber; /**< Subscriber identifier. NULL if no subscriber connected. */
	uint8_t pipeline_cnt; /**< Number of reports in pipeline. Relevant only when connected. */
	uint8_t pipeline_size; /**< Pipeline size. Relevant only when connected. */
};

/** @brief HID report provider function pointers (used by HID state). */
struct hid_report_provider_fops {
	/** @brief Send report to the currently connected subscriber.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] force		Force send report to refresh state.
	 *
	 * @return true, if the report was sent successfully. Otherwise returns false.
	 */
	bool (*send_report)(uint8_t report_id, bool force);

	/** @brief Send empty report to the given subscriber.
	 *
	 * This is an optional callback.
	 *
	 * The empty report is sent to a subscriber different than the currently connected one.
	 * This might be needed to clear state (e.g. clear all of the pressed buttons) when a high
	 * priority subscrier connects and starts receiving HID reports instead of previously
	 * connected low priority subscriber.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] subscriber	Report subscriber.
	 */
	void (*send_empty_report)(uint8_t report_id, const void *subscriber);

	/** @brief Report subscriber connection state to the HID provider.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] cs		HID subscriber connection state.
	 */
	void (*connection_state)(uint8_t report_id, const struct subscriber_conn_state *cs);

	/** @brief Notify that HID report was sent to the subscriber.
	 *
	 * This is an optional callback that allows provider to track number of HID reports in
	 * flight and get information about HID report transmission error.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] error		Report sending error.
	 */
	void (*notify_report_sent)(uint8_t report_id, bool error);
};

/** @brief HID state function pointer (used by HID report provider). */
struct hid_state_fops {
	/** @brief Trigger sending HID report (e.g. when new data is available).
	 *
	 * @param[in] report_id		Report ID.
	 *
	 * @return 0 if the operation was successful. Otherwise, a (negative) error code is
	 * returned.
	 */
	int (*trigger_report_send)(uint8_t report_id);
};

/** @brief HID report provider event. */
struct hid_report_provider_event {
	struct app_event_header header; /**< Event header. */

	const struct hid_report_provider_fops *p_fops; /**< HID provider function pointers. */
	const struct hid_state_fops *hs_fops; /**< HID state function pointers. */
	uint8_t report_id; /**< ID of the report provided by the HID provider. */
};
APP_EVENT_TYPE_DECLARE(hid_report_provider_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HID_REPORT_PROVIDER_EVENT_H_ */
