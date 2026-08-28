/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @addtogroup nrf71_wifi_fw_if Wi-Fi driver and firmware interface
 * @{
 * @brief Coexistence interface between Coexistence Driver and Short-Range Driver
 */

#ifndef __NRF71_CD_SR_IF_H__
#define __NRF71_CD_SR_IF_H__

#include <common/fw_if/pack_def.h>

/** Number of elements in CCCONF priority range buffers. */
#define NUM_ELEMENTS_IN_CCCONF_PTI_RANGE 3U

/** Maximum number of SR CCCONF PTI values. */
#define MAX_NUM_SR_CCCONF_PTI_VALUES 6U

#define COEX_SR_PRIORITY_LEVEL_UNUSED 0xFF

/**
 * SR SW client request status.
 *
 * Indicates whether the SR SW client request was granted or not.
 */
enum coex_sr_sw_client_req_status_t {
	/** Indicates the SR SW client request is granted. */
	SR_SW_CLIENT_REQ_SUCCESS = 0,
	/** Indicates the SR SW client request is NOT granted. */
	SR_SW_CLIENT_REQ_FAIL
};

/**
 * Short Range SW client request type.
 *
 * Indicates the type of SW client operation.
 */
enum coex_sr_sw_client_req_type_t {
	/** Indicates the SW client release. */
	SR_SW_CLIENT_RELEASE = 0,
	/** Indicates the SW client request. */
	SR_SW_CLIENT_REQUEST = 1
};

/**
 * Short Range SW client request priority levels.
 *
 * Indicates the priority level of the SW client request.
 */
enum coex_sr_sw_client_req_pti_level_t {
	/** Low priority level. */
	SR_SW_CLIENT_REQ_PTI_LOW = 0,
	/** Medium priority level. */
	SR_SW_CLIENT_REQ_PTI_MEDIUM,
	/** High priority level. */
	SR_SW_CLIENT_REQ_PTI_HIGH,
	/** Highest priority level. */
	SR_SW_CLIENT_REQ_PTI_HIGHEST,
	/** Total number of priority levels. */
	SR_SW_CLIENT_REQ_PTI_COUNT
};

/**
 * Short-Range power types.
 *
 * Indicates power down and power up scenarios.
 */
enum coex_sr_power_event_t {
	COEX_SR_PREPARE_POWER_DOWN = 0,
	COEX_SR_POWERED_UP_READY
};

/**
 * SR SW client types.
 *
 * Indicates different SR SW clients that can request COEX resources.
 */
enum sr_sw_client_t {
	/** To protect connection phase from Wi-Fi interference. */
	SR_CONNECTION,
	/** To protect calibrations from Wi-Fi interference. */
	SR_CALIBRATIONS,
	/** To protect connection event from Wi-Fi interference. */
	SR_CONNECTION_EVENT,
	/** Total number of SR SW client types. */
	MAX_SR_SW_CLIENT_COUNT
};

/** RF band selection */
enum sr_rf_freq_band_t {
	/** SR operating in 2.4G band */
	SR_BAND_2PT4G,
	/** SR operating in non 2.4G band */
	SR_BAND_NON_2PT4G
};

/**
 * Short-Range activity type
 *
 * This holds the type of Short-Range activity.
 */
enum short_range_activity_type_t {
	/** BLE advertisement . periodic*/
	SR_BLE_ADV = 0,
	/** BLE scan. periodic or continuous based on the parameters */
	SR_BLE_SCAN,
	/** BLE connected data transfer. periodic (connection events) */
	SR_BLE_CONNECTED_DATA_TRANSFER,
	/** Thread discovery .. one shot process*/
	SR_THREAD_DISCOVERY,
	/** Thread end device date transfer .. periodic? depends on application */
	SR_THREAD_ED_DATA_TRANSFER,
	/** Thread end device polling . periodic */
	SR_THREAD_ED_POLLING,
};

/** Short_Range activity action type */
enum short_range_activity_action_t {
	/** Activity start */
	SR_ACTIVITY_START,
	/**  Activity end */
	SR_ACTIVITY_END
};

/**
 * Short-Range activity information
 *
 * This structure holds the activity of Short-Range radios and the parameters required
 * for generating Periodic Priority Windows (PPWs) for Wi-Fi and SR radios.
 */
struct short_range_activity_info_t {
	/** Short-Range activity type. see &enum short_range_activity_type_t */
	unsigned int sr_activity_type;
	/** Short-Range activity action: start or end. see &enum short_range_activity_action_t */
	unsigned int sr_activity_action;
	/*
	 * Start time of the periodic activity. An absolute timestamp in the common GRTC domain,
	 * expressed in milliseconds and rounded down to the nearest integer.
	 * Set to zero for a non-aligned activity.
	 */
	unsigned int start_time_of_activity;

	/** Short-Range activity parameters */
	/** Activity periodicity/interval in milliseconds*/
	unsigned int activity_interval;
	/** Activity duration (ON time , also called active window) in milliseconds */
	unsigned int activity_duration;
	/** Activity predefined timeout */
	unsigned int activity_timeout;
	/** Reserved for future use */
	unsigned int parameter4;
	/** Reserved for future use */
	unsigned int parameter5;
} __NRF_WIFI_PKD;

/**
 * SR clients priority range values.
 *
 * Contains SR clients priority range values for coexistence configuration.
 * Each range is defined as {start, end, step}, where start is numerically
 * greater than end. The Coexistence Manager uses only the regular SR Rx/Tx
 * ranges; the critical ranges are consumed by the Short-Range driver.
 */
struct coex_sr_priority_range_t {
	/** SR Rx client CCCONF priority range. */
	unsigned char sr_rx_client_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** SR Tx client CCCONF priority range. */
	unsigned char sr_tx_client_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** Critical-activity SR Rx client CCCONF priority range. */
	unsigned char sr_rx_client_critical_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/** Critical-activity SR Tx client CCCONF priority range. */
	unsigned char sr_tx_client_critical_ccconf_pti_range[NUM_ELEMENTS_IN_CCCONF_PTI_RANGE];
	/**
	 * Optional index into the applicable populated priority array. When
	 * equal to COEX_SR_PRIORITY_LEVEL_UNUSED (0xFF), the SR driver selects
	 * a priority value according to packet type and activity criticality.
	 */
	unsigned char client_priority_level;
} __NRF_WIFI_PKD;

/**
 * Short Range Software client request parameters
 *
 * This structure holds the parameters required to post
 * a SW client request to request COEX resources.
 */
struct coex_sr_sw_client_params_t {
	/** SR SW client request/release. see &enum coex_sr_sw_client_req_type_t */
	unsigned int sw_client_request;
	/** SW client priority level. see &enum coex_sr_sw_client_req_pti_level_t */
	unsigned int sw_client_pti_level;
	/** SW client type. see &enum sr_sw_client_t */
	unsigned int sw_client_type;
	/** SW request timeout in milliseconds */
	unsigned int request_timeout_in_ms;
	/** SR operating band. see &enum sr_rf_freq_band_t */
	unsigned int sr_operating_band;
} __NRF_WIFI_PKD;

/**
 * Enable or disable posting of SR hardware client requests to COEXC.
 *
 * Returns true when the requested state was applied.
 */
unsigned int coex_sr_enable(unsigned int enable_coex);

/**
 * Replace the locally stored SR Rx/Tx priority ranges.
 *
 * Returns true when both ranges and client_priority_level are valid and stored.
 */
unsigned int coex_sr_set_client_priority(const struct coex_sr_priority_range_t *sr_priority_range);

/**
 * Request or release an SR Single Priority Window through CD.
 *
 * Returns zero on successful command processing and a negative errno value on
 * validation, state, transport, or response-timeout failure. On a zero return,
 * grant_status contains the CM grant decision.
 */
int coex_cd_sr_software_client_request(const struct coex_sr_sw_client_params_t *client_params,
				       enum coex_sr_sw_client_req_status_t *grant_status);

/**
 * Report SR activity information used by CD for PPW selection and generation.
 *
 * Returns zero on success or a negative errno value.
 */
int coex_cd_update_short_range_activity_info(
	const struct short_range_activity_info_t *activity_info);

/**
 * Notify CD before SR power-down and after SR is ready following power-up.
 *
 * Returns zero on success or a negative errno value.
 */
int coex_cd_sr_power_notify(enum coex_sr_power_event_t event);

/**
 * @}
 */
#endif /* __NRF71_CD_SR_IF_H__ */
