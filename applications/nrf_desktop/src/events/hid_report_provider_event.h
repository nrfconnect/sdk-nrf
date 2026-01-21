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
 * @brief HID report provider events
 * @defgroup nrf_desktop_hid_report_provider_event HID report provider events
 *
 * The @ref hid_report_provider_event is used to link the HID report provider and the HID state
 * during boot.
 *
 * The HID state module tracks the state of HID report subscribers (HID transports) and notifies the
 * HID report providers when the HID report subscription state changes. The HID state also
 * determines the active HID report subscriber (based on HID subscriber priorities) and limits the
 * number of HID reports that are processed by a HID transport at a given time.
 *
 * The HID report providers aggregate data related to user input, form the HID input reports, and
 * forward the HID input reports to HID transports as a @ref hid_report_event when requested by the
 * HID state.
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

/** @brief HID report provider API (used by HID state). */
struct hid_report_provider_api {
	/** @brief Send report to the currently connected subscriber.
	 *
	 * The HID provider must send the HID report as a @ref hid_report_event only if the HID
	 * report data requires an update (there is user input waiting). Otherwise, the HID provider
	 * must skip sending the HID report unless a force flag is set.
	 *
	 * If the force flag is set, the HID provider needs to always send a HID report (even if the
	 * report would contain no new information). The HID state sets the force flag if the
	 * connected subscriber requires an update (to refresh the state).
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
	 * priority subscriber connects and starts receiving HID reports instead of previously
	 * connected low priority subscriber.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] subscriber	Report subscriber.
	 */
	void (*send_empty_report)(uint8_t report_id, const void *subscriber);

	/** @brief Report subscriber connection state change to the HID provider.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] cs		HID subscriber connection state. The value under the pointer
	 *				may not be valid after the function returns.
	 */
	void (*connection_state_changed)(uint8_t report_id, const struct subscriber_conn_state *cs);

	/** @brief Notify HID provider that HID report was sent to the subscriber.
	 *
	 * This is an optional callback that allows provider to track number of HID reports in
	 * flight and get information about HID report transmission error.
	 *
	 * @param[in] report_id		Report ID.
	 * @param[in] error		Report sending error.
	 */
	void (*report_sent)(uint8_t report_id, bool error);
};

/** @brief HID state APIs (used by HID report provider). */
struct hid_state_api {
	/** @brief Trigger sending HID report to the currently connected subscriber.
	 *
	 * The function is used to notify the HID state that new data is available (for example on
	 * user input). The HID provider needs to wait with submitting a @ref hid_report_event until
	 * the HID state requests it through the send_report callback.
	 *
	 * @param[in] report_id		Report ID.
	 *
	 * @return 0 if the operation was successful. Otherwise, a (negative) error code is
	 * returned.
	 */
	int (*trigger_report_send)(uint8_t report_id);
};

/** @brief HID report provider event.
 *
 * The HID report provider submits the event to pass a HID report ID and provider API to the HID
 * state. While processing the event, the HID state remembers the data received from the HID report
 * provider and fills the HID state API. The HID report provider must process the event after the
 * HID state to receive the HID state API.
 *
 * The exchanged API structures allow for direct function calls between the application modules at
 * runtime (without using application events). The direct calls are used to ensure high performance.
 */
struct hid_report_provider_event {
	struct app_event_header header; /**< Event header. */

	const struct hid_report_provider_api *provider_api; /**< HID report provider API. */
	const struct hid_state_api *hid_state_api; /**< HID state API. */
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
