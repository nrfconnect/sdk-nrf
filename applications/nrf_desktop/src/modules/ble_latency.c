/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

#include <caf/events/ble_common_event.h>
#include <caf/events/ble_smp_event.h>
#include "config_event.h"
#include "hids_event.h"
#include <caf/events/power_event.h>

#define MODULE ble_latency
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_LATENCY_LOG_LEVEL);

#define SECURITY_FAIL_TIMEOUT_MS \
	K_SECONDS(CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S)
#define INIT_CONN_PARAMS_UPDATE_TIMEOUT_MS K_SECONDS(5)
#define LOW_LATENCY_CHECK_PERIOD_MS	K_SECONDS(5)
#define DEFAULT_LATENCY			CONFIG_BT_PERIPHERAL_PREF_LATENCY
#define DEFAULT_TIMEOUT			CONFIG_BT_PERIPHERAL_PREF_TIMEOUT
/* Connection intervals used by LLPM are out of Bluetooth LE specification.
 * The intervals are encoded by OR operation with a magic number of 0x0d00.
 */
#define REG_CONN_INTERVAL_LLPM_MASK	0x0d00
#define REG_CONN_INTERVAL_BLE_DEFAULT	0x0006

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS),
	     "CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS must be disabled for "
	     "nRF Desktop peripherals");

static struct bt_conn *active_conn;
static struct k_work_delayable security_timeout;
static struct k_work_delayable low_latency_check;
static struct k_work_delayable init_conn_params;
static struct k_work_delayable init_conn_params_update_timeout;

enum {
	CONN_LOW_LATENCY_ENABLED	= BIT(0),
	CONN_LOW_LATENCY_REQUIRED	= BIT(1),
	CONN_LOW_LATENCY_LOCKED		= BIT(2),
	CONN_IS_LLPM			= BIT(3),
	CONN_IS_SCI			= BIT(4),
	CONN_IS_SECURED			= BIT(5),
	CONN_IS_SCI_PARAM_UPDATE_PENDING = BIT(6),
	CONN_IS_SCI_OUT_OF_SPEC	= BIT(7),
	CONN_IS_POWER_DOWN		= BIT(8),
	CONN_IS_SCI_LOW_POWER_HIGH_INTERVAL_FORBIDDEN = BIT(9),
	CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS = BIT(10),
};

static uint16_t latency_state;

static enum bt_hids_sci_mode_value processed_sci_mode = BT_HIDS_SCI_MODE_NONE;
static enum bt_hids_sci_mode_value last_requested_sci_mode = BT_HIDS_SCI_MODE_NONE;
static enum bt_hids_sci_mode_value host_requested_sci_mode = BT_HIDS_SCI_MODE_NONE;
static bool last_requested_latency_is_low;

static void security_timeout_fn(struct k_work *w)
{
	BUILD_ASSERT(CONFIG_BT_MAX_CONN == 1);
	BUILD_ASSERT(CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S > 0);
	__ASSERT_NO_MSG(active_conn);

	int err = bt_conn_disconnect(active_conn,
				     BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	LOG_WRN("Security establishment failed");

	if (err && (err != -ENOTCONN)) {
		LOG_ERR("Cannot disconnect peer (err=%d)", err);
		module_set_state(MODULE_STATE_ERROR);
	} else {
		LOG_INF("Peer disconnected");
	}
}

static void set_init_conn_params(void)
{
	const struct bt_le_conn_param param = {
		.interval_min = REG_CONN_INTERVAL_BLE_DEFAULT,
		.interval_max = REG_CONN_INTERVAL_BLE_DEFAULT,
		.latency = 0,
		.timeout = DEFAULT_TIMEOUT
	};

	int err = bt_conn_le_param_update(active_conn, &param);
	last_requested_latency_is_low = true;

	if (!err) {
		latency_state |= CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS;
		/* The conn_params_update_rejected callback was not
		 * available on NCS 3.4.0 which this code needs to be based on.
		 * To avoid the CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS being set
		 * permanently locking connection rate updates, treat the update
		 * as rejected/failed after the INIT_CONN_PARAMS_UPDATE_TIMEOUT_MS
		 * timeout expires.
		 * This workaround should be removed when the callback is available
		 * to nRF Desktop.
		 */
		(void)k_work_schedule(&init_conn_params_update_timeout,
				      INIT_CONN_PARAMS_UPDATE_TIMEOUT_MS);
	}

	if (!err || (err == -EALREADY)) {
		LOG_INF("Init connection parameters are set");
	} else {
		LOG_WRN("Failed to update conn parameters (err %d)", err);
	}
}

static void init_conn_params_fn(struct k_work *w)
{
	ARG_UNUSED(w);

	__ASSERT_NO_MSG(active_conn);

	set_init_conn_params();
}

static void set_conn_latency_non_sci(bool low_latency)
{
	struct bt_conn_info info;

	int err = bt_conn_get_info(active_conn, &info);

	if (err) {
		LOG_WRN("Cannot get conn info (%d)", err);
		return;
	}

	__ASSERT_NO_MSG(info.role == BT_CONN_ROLE_PERIPHERAL);
	if ((low_latency && (info.le.latency == 0)) ||
	    ((!low_latency) && (info.le.latency == DEFAULT_LATENCY))) {
		LOG_INF("Latency is already updated");
		return;
	}

	/* Request with connection interval set to a LLPM value is rejected
	 * by Zephyr Bluetooth API.
	 */
	uint16_t info_interval_1250us = BT_GAP_US_TO_CONN_INTERVAL(info.le.interval_us);
	uint16_t interval = (info_interval_1250us & REG_CONN_INTERVAL_LLPM_MASK) ?
			 REG_CONN_INTERVAL_BLE_DEFAULT : info_interval_1250us;
	const struct bt_le_conn_param param = {
		.interval_min = interval,
		.interval_max = interval,
		.latency = (low_latency) ? (0) : (DEFAULT_LATENCY),
		.timeout = info.le.timeout
	};

	err = bt_conn_le_param_update(active_conn, &param);

	if (!err || (err == -EALREADY)) {
		LOG_INF("BLE latency %screased", low_latency ? "de" : "in");

		if (low_latency) {
			latency_state |= CONN_LOW_LATENCY_ENABLED;
		} else {
			latency_state &= ~CONN_LOW_LATENCY_ENABLED;
		}
	} else {
		LOG_WRN("Failed to update conn parameters (err %d)", err);
	}
}

static int hid_sci_conn_rate_request(bool low_latency, enum bt_hids_sci_mode_value mode)
{
	int err = 0;

	__ASSERT_NO_MSG(active_conn);

	const struct bt_conn_le_conn_rate_param *mode_params =
		bt_hids_sci_mode_conn_rate_param_get(mode);
	struct bt_conn_le_conn_rate_param params;

	__ASSERT_NO_MSG(mode_params);

	params = *mode_params;

	if (low_latency) {
		params.max_latency = 0;
	}

	if (mode == BT_HIDS_SCI_MODE_LOW_POWER &&
		!(latency_state & CONN_IS_SCI_LOW_POWER_HIGH_INTERVAL_FORBIDDEN)) {
		/* Attempt to request the highest interval possible for the LOW_POWER mode.
		 * This is done to reduce power consumption as much as possible.
		 * Most hosts will support the maximum LOW_POWER mode interval.
		 * If negotiation fails, the CONN_IS_SCI_LOW_POWER_HIGH_INTERVAL_FORBIDDEN
		 * flag is set, and no more such attempts are made until BLE link disconnects.
		 */
		params.interval_min_125us = params.interval_max_125us;
	}

	err = bt_conn_le_conn_rate_request(active_conn, &params);

	if (!err) {
		processed_sci_mode = mode;
	} else {
		LOG_ERR("HID SCI connection rate request failed (%d)", err);
	}

	return err;
}

static void set_conn_latency_sci(bool low_latency)
{
	enum bt_hids_sci_mode_value current_mode;
	int err = bt_hids_sci_mode_get(active_conn, &current_mode);

	if (err) {
		LOG_ERR("Failed to get current SCI mode (%d)", err);
		return;
	}

	if ((latency_state & CONN_LOW_LATENCY_LOCKED) ||
		(latency_state & CONN_IS_SCI_OUT_OF_SPEC)) {
		return;
	}

	if (current_mode == BT_HIDS_SCI_MODE_NONE) {
		LOG_WRN("Skipping latency update for an SCI connection with parameters "
			"that do not fit any HID SCI mode");
		return;
	}

	if (processed_sci_mode != BT_HIDS_SCI_MODE_NONE) {
		/* A connection rate update is already in progress */
		latency_state |= CONN_IS_SCI_PARAM_UPDATE_PENDING;
		return;
	}

	err = hid_sci_conn_rate_request(low_latency, current_mode);

	if (!err) {
		LOG_INF("BLE latency %screased", low_latency ? "de" : "in");

		if (low_latency) {
			latency_state |= CONN_LOW_LATENCY_ENABLED;
		} else {
			latency_state &= ~CONN_LOW_LATENCY_ENABLED;
		}
	}
}

static void set_conn_latency(bool low_latency)
{
	if (!active_conn) {
		return;
	}

	last_requested_latency_is_low = low_latency;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
	    (latency_state & CONN_IS_SCI)) {
		set_conn_latency_sci(low_latency);
	} else {
		set_conn_latency_non_sci(low_latency);
	}
}

static void update_llpm_conn_latency_lock(void)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK) ||
	    !(latency_state & CONN_IS_LLPM)) {
		return;
	}

	if (latency_state & CONN_LOW_LATENCY_LOCKED) {
		if (!(latency_state & CONN_LOW_LATENCY_ENABLED)) {
			set_conn_latency(true);
		}
		/* Cancel cannot fail if executed from another work's context. */
		(void)k_work_cancel_delayable(&low_latency_check);
	} else if (latency_state & CONN_LOW_LATENCY_ENABLED) {
		latency_state &= ~CONN_LOW_LATENCY_REQUIRED;
		k_work_reschedule(&low_latency_check,
				      LOW_LATENCY_CHECK_PERIOD_MS);
	}
}

static void low_latency_check_fn(struct k_work *w)
{
	__ASSERT_NO_MSG(!((latency_state & CONN_LOW_LATENCY_LOCKED) &&
			  (latency_state & CONN_IS_LLPM)));

	/* Do not increase slave latency until the connection is secured.
	 * Increasing slave latency may significantly increase time
	 * required to establish security on some hosts.
	 */
	if (!(latency_state & CONN_IS_SECURED)) {
		k_work_reschedule(&low_latency_check,
				      LOW_LATENCY_CHECK_PERIOD_MS);
	} else if (latency_state & CONN_LOW_LATENCY_REQUIRED) {
		latency_state &= ~CONN_LOW_LATENCY_REQUIRED;
		k_work_reschedule(&low_latency_check,
				      LOW_LATENCY_CHECK_PERIOD_MS);
	} else {
		LOG_INF("Low latency timed out");
		set_conn_latency(false);
	}
}

static bool is_high_latency_supported_for_mode(enum bt_hids_sci_mode_value mode)
{
	const struct bt_conn_le_conn_rate_param *mode_params = NULL;

	/* HID SCI mode NONE could only happen if we are in an "out-of-spec" state.
	 * No latency updates are supported in this state.
	 */
	if (mode != BT_HIDS_SCI_MODE_NONE) {
		mode_params = bt_hids_sci_mode_conn_rate_param_get(mode);
	}

	return (mode_params && (mode_params->max_latency > 0));
}

static void latency_updated(bool low_latency)
{
	if (low_latency) {
		latency_state |= CONN_LOW_LATENCY_ENABLED;
		latency_state &= ~CONN_LOW_LATENCY_REQUIRED;
		if (!(latency_state & CONN_LOW_LATENCY_LOCKED)
			&& !(latency_state & CONN_IS_SCI_OUT_OF_SPEC)) {
			(void)k_work_reschedule(&low_latency_check,
						LOW_LATENCY_CHECK_PERIOD_MS);
		} else {
			(void)k_work_cancel_delayable(&low_latency_check);
		}
	} else {
		latency_state &= ~CONN_LOW_LATENCY_ENABLED;
		/* Cancel cannot fail if executed from another work's context. */
		(void)k_work_cancel_delayable(&low_latency_check);
	}
}

static void pending_sci_conn_rate_update_handle(void);

static void conn_params_update_finished_sci_conn(void)
{
	if (latency_state & CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS) {
		if (latency_state & CONN_IS_SCI_PARAM_UPDATE_PENDING) {
			pending_sci_conn_rate_update_handle();
		}
	} else {
		LOG_WRN("Unexpected non-SCI connection parameters update while SCI is active");

		latency_state &= ~CONN_IS_SCI_PARAM_UPDATE_PENDING;
		latency_state |= CONN_IS_SCI_OUT_OF_SPEC;
		(void)k_work_cancel_delayable(&low_latency_check);
	}

	latency_state &= ~CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS;
}

static void init_conn_params_update_timeout_fn(struct k_work *w)
{
	ARG_UNUSED(w);

	__ASSERT_NO_MSG(active_conn);
	__ASSERT_NO_MSG(latency_state & CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS);

	LOG_WRN("Connection parameter update timed out");

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
	    (latency_state & CONN_IS_SCI)) {
		conn_params_update_finished_sci_conn();
		return;
	}

	latency_state &= ~CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS;
}

static void conn_params_updated(const struct ble_peer_conn_params_event *event)
{
	if (!event->updated) {
		/* Ignore the connection parameters update request. */
		return;
	}

	(void)k_work_cancel_delayable(&init_conn_params_update_timeout);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
	    (latency_state & CONN_IS_SCI)) {
		conn_params_update_finished_sci_conn();
		return;
	}
	latency_state &= ~CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS;

	__ASSERT_NO_MSG(event->interval_min == event->interval_max);

	if (event->interval_min & REG_CONN_INTERVAL_LLPM_MASK) {
		latency_state |= CONN_IS_LLPM;
	} else {
		latency_state &= ~CONN_IS_LLPM;
	}

	latency_updated(event->latency == 0);

	update_llpm_conn_latency_lock();
}

static const char *sci_mode_to_string(enum bt_hids_sci_mode_value mode)
{
	switch (mode) {
	case BT_HIDS_SCI_MODE_NONE:
		return "NONE";
	case BT_HIDS_SCI_MODE_DEFAULT:
		return "DEFAULT";
	case BT_HIDS_SCI_MODE_FAST:
		return "FAST";
	case BT_HIDS_SCI_MODE_LOW_POWER:
		return "LOW_POWER";
	case BT_HIDS_SCI_MODE_FULL_RANGE:
		return "FULL_RANGE";
	default:
		break;
	}

	return "UNKNOWN";
}

static void hid_sci_mode_request(enum bt_hids_sci_mode_value mode)
{
	if (!active_conn) {
		return;
	}

	__ASSERT_NO_MSG(mode != BT_HIDS_SCI_MODE_NONE);

	if (latency_state & CONN_IS_POWER_DOWN) {
		/* Force the SCI mode to LOW_POWER if the peripheral is in power down/suspended
		 * state, regardless of the HID SCI mode requested by the host.
		 * Usually in this case the peripheral will already be in the LOW_POWER SCI mode,
		 * so the code below will result in no connection rate update and no SCI mode change
		 * notification to the host.
		 * The notable exception is if the peripheral is operating in an out-of-spec state,
		 * due to a previous direct connection rate update from the host.
		 * In this case, the current mode might be different - an attempt will be made to
		 * switch to the LOW_POWER mode, and only if it succeeds, the peripheral will exit
		 * the out-of-spec state.
		 */
		mode = BT_HIDS_SCI_MODE_LOW_POWER;
	}

	last_requested_sci_mode = mode;
	/* Once the host issues a HID SCI mode request or uses the connection
	 * rate API to set the connection parameters, the peripheral switches
	 * to using the SCI API for this connection.
	 * After that point, the non-SCI connection parameters API is no longer used.
	 */
	latency_state |= CONN_IS_SCI;
	(void)k_work_cancel_delayable(&init_conn_params);

	if ((processed_sci_mode != BT_HIDS_SCI_MODE_NONE) ||
	    (latency_state & CONN_IS_INIT_PARAMS_UPDATE_IN_PROGRESS)) {
		/* A connection rate/params update is already in progress */
		latency_state |= CONN_IS_SCI_PARAM_UPDATE_PENDING;
		return;
	}

	enum bt_hids_sci_mode_value current_mode;
	int err = bt_hids_sci_mode_get(active_conn, &current_mode);

	if (err) {
		LOG_ERR("Failed to get current SCI mode (%d)", err);
		return;
	}

	if (current_mode != mode) {
		(void)hid_sci_conn_rate_request(last_requested_latency_is_low, mode);
	} else if (latency_state & CONN_IS_SCI_OUT_OF_SPEC) {
		struct bt_conn_info info;

		/* Mode update properly requested after out-of-spec behavior,
		 * but the requested mode matches the current mode (no connection
		 * rate update is performed).
		 */
		latency_state &= ~CONN_IS_SCI_OUT_OF_SPEC;
		LOG_INF("The current connection parameters are valid for the requested mode %s",
			sci_mode_to_string(mode));
		LOG_INF("Clearing out-of-spec state and returning to normal operation");
		err = bt_conn_get_info(active_conn, &info);

		if (!err) {
			latency_updated(info.le.latency == 0);
		} else {
			LOG_ERR("Failed to get connection info: %d", err);
		}
	} else {
		/* Do nothing. */
	}
}

static enum bt_hids_sci_mode_value
find_valid_sci_mode(const struct bt_conn_le_conn_rate_changed *params)
{
	static const enum bt_hids_sci_mode_value modes_to_try[] = {
		BT_HIDS_SCI_MODE_FAST,
		BT_HIDS_SCI_MODE_DEFAULT,
		BT_HIDS_SCI_MODE_LOW_POWER,
		BT_HIDS_SCI_MODE_FULL_RANGE,
	};
	enum bt_hids_sci_mode_value valid_mode = BT_HIDS_SCI_MODE_NONE;

	LOG_DBG("Attempting to find a valid SCI mode...");

	for (size_t i = 0; i < ARRAY_SIZE(modes_to_try); i++) {
		if (bt_hids_sci_mode_validate(modes_to_try[i], params)) {
			valid_mode = modes_to_try[i];
			LOG_INF("Found valid SCI mode: %s", sci_mode_to_string(valid_mode));
			break;
		}
	}

	if (valid_mode == BT_HIDS_SCI_MODE_NONE) {
		LOG_WRN("No valid SCI mode found. Defaulting to NONE");
	}

	return valid_mode;
}

static enum bt_hids_sci_mode_value
get_new_sci_mode(const struct bt_conn_le_conn_rate_changed *params,
		 enum bt_hids_sci_mode_value preferred_sci_mode,
		 enum bt_hids_sci_mode_value current_sci_mode)
{
	bool mode_ok = false;
	enum bt_hids_sci_mode_value mode = preferred_sci_mode;

	/* In case a connection rate change is received without a prior HID SCI mode change request,
	 * or if a HID SCI mode change was requested but the new connection rate parameters
	 * are not valid for the requested mode, this application first checks if the new parameters
	 * are valid for the current SCI mode.
	 * If not, it attempts to find a valid SCI mode by cycling through the available SCI modes
	 * from the most restrictive (FAST) to the least restrictive (FULL_RANGE) mode.
	 * If no valid SCI mode is found, the application defaults to the NONE mode.
	 *
	 * This is an application-specific behavior, not enforced by the specification.
	 */
	if (mode != BT_HIDS_SCI_MODE_NONE) {
		mode_ok = bt_hids_sci_mode_validate(mode, params);
	}

	if (!mode_ok && (current_sci_mode != BT_HIDS_SCI_MODE_NONE)) {
		LOG_INF("Validating parameters for the current mode %s",
			sci_mode_to_string(current_sci_mode));

		mode_ok = bt_hids_sci_mode_validate(current_sci_mode, params);

		if (mode_ok) {
			LOG_DBG("Parameters are valid for the current mode. "
				"No action needed.");
			mode = current_sci_mode;
		} else {
			LOG_DBG("Invalid SCI mode parameters for current mode.");
		}
	}

	if (!mode_ok) {
		mode = find_valid_sci_mode(params);
	}

	return mode;
}

static void pending_sci_conn_rate_update_handle(void)
{
	struct bt_conn_info info;
	int err = bt_conn_get_info(active_conn, &info);
	bool is_low_latency;

	if (err) {
		LOG_ERR("Failed to get connection info: %d", err);
		return;
	}

	is_low_latency = (info.le.latency == 0);

	bool latency_request_pending = (last_requested_latency_is_low != is_low_latency);
	enum bt_hids_sci_mode_value current_mode = BT_HIDS_SCI_MODE_NONE;

	err = bt_hids_sci_mode_get(active_conn, &current_mode);

	if (err) {
		LOG_ERR("Failed to get current SCI mode: %d", err);
		return;
	}

	bool mode_update_pending = (last_requested_sci_mode != current_mode);

	if (!last_requested_latency_is_low
	    && !is_high_latency_supported_for_mode(last_requested_sci_mode)) {
		latency_request_pending = false;
		last_requested_latency_is_low = true;
	}

	if ((last_requested_sci_mode != BT_HIDS_SCI_MODE_NONE) &&
	    (mode_update_pending || latency_request_pending)) {
		if (mode_update_pending) {
			LOG_DBG("Pending HID SCI mode update detected");
		}
		if (latency_request_pending) {
			LOG_DBG("Pending latency request detected");
		}
		LOG_DBG("Pending mode: %s", sci_mode_to_string(last_requested_sci_mode));
		LOG_DBG("Pending latency: %s", last_requested_latency_is_low ? "low" : "high");

		(void)hid_sci_conn_rate_request(last_requested_latency_is_low,
						last_requested_sci_mode);
	}

	/* Avoid cyclic requests for a pending parameters update if the previous
	 * request failed or did not take effect.
	 */
	latency_state &= ~CONN_IS_SCI_PARAM_UPDATE_PENDING;
}

static void conn_rate_update_success_handle(const struct ble_peer_sci_conn_rate_event *event)
{
	enum bt_hids_sci_mode_value mode;
	int err = 0;
	enum bt_hids_sci_mode_value current_sci_mode = BT_HIDS_SCI_MODE_NONE;

	LOG_DBG("Connection rate changed:");
	LOG_DBG(" interval: %" PRIu32 " us", event->params.interval_us);
	LOG_DBG(" subrate: %" PRIu16, event->params.subrate_factor);
	LOG_DBG(" peripheral latency: %" PRIu16, event->params.peripheral_latency);
	LOG_DBG(" continuation number: %" PRIu16, event->params.continuation_number);
	LOG_DBG(" supervision timeout: %" PRIu16 " (10ms)",
		event->params.supervision_timeout_10ms);

	err = bt_hids_sci_mode_get(active_conn, &current_sci_mode);
	if (err) {
		LOG_ERR("Failed to get current SCI mode: %d", err);
		current_sci_mode = BT_HIDS_SCI_MODE_NONE;
	}

	mode = get_new_sci_mode(&event->params, processed_sci_mode, current_sci_mode);

	if (mode != current_sci_mode) {
		err = bt_hids_sci_mode_updated(active_conn, mode);
		if (!err) {
			LOG_INF("Updated to the SCI mode: %s", sci_mode_to_string(mode));
		} else {
			LOG_ERR("Failed to notify about new HID SCI mode: %d", err);
		}
	}

	if (is_high_latency_supported_for_mode(mode)) {
		latency_state &= ~CONN_LOW_LATENCY_LOCKED;
	} else {
		latency_state |= CONN_LOW_LATENCY_LOCKED;
	}

	/* Parameters update initiated directly by the host (not via SCI
	 * mode API) is out-of-spec behavior.
	 * We assume that if the host forces the parameters, it expects the peripheral
	 * to keep them.
	 * Thus, lock the possibility of autonomously changing connection latency.
	 * Also, drop any pending parameters update request.
	 * If these actions are not taken, repeated failed connection rate updates
	 * could be triggered by the peripheral.
	 */
	if (processed_sci_mode == BT_HIDS_SCI_MODE_NONE) {
		LOG_WRN("Direct SCI connection parameters update from the central");
		LOG_WRN("(not through HID SCI control point characteristic).");
		LOG_WRN("This is out-of-spec behavior.");
		LOG_WRN("No latency update requests will be performed by the peripheral "
			"until a SCI mode request is received.");
		latency_state &= ~CONN_IS_SCI_PARAM_UPDATE_PENDING;
		latency_state |= CONN_IS_SCI_OUT_OF_SPEC;
		(void)k_work_cancel_delayable(&low_latency_check);
	} else if (mode == processed_sci_mode) {
		/* A properly requested SCI mode update has been successfully performed. */
		if (latency_state & CONN_IS_SCI_OUT_OF_SPEC) {
			LOG_INF("Connection rate update successful for the requested mode %s",
				sci_mode_to_string(mode));
			LOG_INF("Clearing out-of-spec state and returning to normal operation");
			latency_state &= ~CONN_IS_SCI_OUT_OF_SPEC;
		}
	} else {
		/* Do nothing, remain in the in-spec/out-of-spec state. */
	}
}

static void conn_rate_updated(const struct ble_peer_sci_conn_rate_event *event)
{
	if (event->id != active_conn) {
		LOG_WRN("Conn rate updated event for a non-active connection");
		return;
	}

	if (!(latency_state & CONN_IS_SCI)) {
		LOG_WRN("Connection rate API used without a prior SCI mode request");
		LOG_WRN("Peripheral will start using SCI and attempt to find a valid SCI mode");
		latency_state |= CONN_IS_SCI;
		(void)k_work_cancel_delayable(&init_conn_params);
	}

	if (event->status == BT_HCI_ERR_SUCCESS) {
		conn_rate_update_success_handle(event);
	} else {
		LOG_WRN("Connection rate change failed (status: 0x%02x)", event->status);

		if (((event->status == BT_HCI_ERR_UNSUPP_LL_PARAM_VAL) ||
		     (event->status == BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL) ||
		     (event->status == BT_HCI_ERR_UNACCEPT_CONN_PARAM) ||
		     (event->status == BT_HCI_ERR_INVALID_LL_PARAM)) &&
		    (processed_sci_mode == BT_HIDS_SCI_MODE_LOW_POWER) &&
		    !(latency_state & CONN_IS_SCI_LOW_POWER_HIGH_INTERVAL_FORBIDDEN)) {
			/* For properly configured peripheral and host devices
			 * the only reason we could get here is on the first LOW_POWER mode
			 * request, which attempts to request connection rate with min interval
			 * equal to the maximum interval.
			 */
			latency_state |= CONN_IS_SCI_LOW_POWER_HIGH_INTERVAL_FORBIDDEN;
			LOG_INF("Max LOW_POWER mode interval not supported for this connection.");
			LOG_INF("No more attempts will be made to request the max LOW_POWER "
				"interval for this connection.");

			/* If a new mode update is already pending, this will have no effect.
			 * Otherwise, LOW_POWER parameters with full LOW_POWER interval range
			 * will be requested.
			 */
			latency_state |= CONN_IS_SCI_PARAM_UPDATE_PENDING;
		}
	}

	processed_sci_mode = BT_HIDS_SCI_MODE_NONE;

	struct bt_conn_info info;

	int err = bt_conn_get_info(active_conn, &info);

	if (!err) {
		bool is_low_latency = (info.le.latency == 0);

		latency_updated(is_low_latency);

		if (latency_state & CONN_IS_SCI_PARAM_UPDATE_PENDING) {
			pending_sci_conn_rate_update_handle();
		}
	} else {
		LOG_ERR("Failed to get connection info: %d", err);
	}
}

static void sci_power_event_handle(void)
{
	if (!active_conn) {
		return;
	}

	if (latency_state & CONN_IS_SCI_OUT_OF_SPEC) {
		return;
	}

	__ASSERT_NO_MSG(latency_state & CONN_IS_SCI);

	if (latency_state & CONN_IS_POWER_DOWN) {
		hid_sci_mode_request(BT_HIDS_SCI_MODE_LOW_POWER);
	} else if (host_requested_sci_mode != BT_HIDS_SCI_MODE_NONE) {
		hid_sci_mode_request(host_requested_sci_mode);
	} else {
		/* Do nothing. */
	}
}

static void init(void)
{
	k_work_init_delayable(&security_timeout, security_timeout_fn);
	k_work_init_delayable(&low_latency_check, low_latency_check_fn);
	k_work_init_delayable(&init_conn_params, init_conn_params_fn);
	k_work_init_delayable(&init_conn_params_update_timeout,
			      init_conn_params_update_timeout_fn);
}

static void use_low_latency(void)
{
	if (!active_conn) {
		return;
	}

	if (latency_state & CONN_LOW_LATENCY_ENABLED) {
		latency_state |= CONN_LOW_LATENCY_REQUIRED;
	} else {
		set_conn_latency(true);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			init();
			__ASSERT_NO_MSG(!initialized);
			initialized = true;
		}

		return false;
	}

	if (is_ble_peer_event(aeh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(aeh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
			active_conn = event->id;
			if (IS_ENABLED(CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK)) {
				latency_state |= CONN_LOW_LATENCY_LOCKED;
			}

			(void)k_work_schedule(&init_conn_params,
						  K_MSEC(CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT));
			/* This is needed so that low latency is used if the remote
			 * requests a HID SCI mode before the init conn params work
			 * is executed.
			 * Low latency speeds up security establishment and GATT discovery.
			 */
			last_requested_latency_is_low = true;

			k_work_reschedule(&security_timeout,
					      SECURITY_FAIL_TIMEOUT_MS);
			break;

		case PEER_STATE_DISCONNECTED:
			__ASSERT_NO_MSG(active_conn == event->id);
			active_conn = NULL;

			/* Clear BLE latency state. */
			latency_state = 0;
			last_requested_latency_is_low = false;

			if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE)) {
				processed_sci_mode = BT_HIDS_SCI_MODE_NONE;
				last_requested_sci_mode = BT_HIDS_SCI_MODE_NONE;
				host_requested_sci_mode = BT_HIDS_SCI_MODE_NONE;
			}

			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&init_conn_params);
			(void)k_work_cancel_delayable(&init_conn_params_update_timeout);
			(void)k_work_cancel_delayable(&low_latency_check);
			(void)k_work_cancel_delayable(&security_timeout);
			break;

		case PEER_STATE_SECURED:
			latency_state |= CONN_IS_SECURED;
			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&security_timeout);
			break;

		default:
			/* Ignore. */
			break;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    is_config_event(aeh)) {
		use_low_latency();

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_SMP_TRANSFER_EVENTS) &&
	    is_ble_smp_transfer_event(aeh)) {
		use_low_latency();

		return false;
	}

	if (is_ble_peer_conn_params_event(aeh)) {
		conn_params_updated(cast_ble_peer_conn_params_event(aeh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
	    is_hid_sci_mode_request_event(aeh)) {
		const struct hid_sci_mode_request_event *event =
			cast_hid_sci_mode_request_event(aeh);

		if (event->subscriber != active_conn) {
			LOG_WRN("HID SCI mode request event for a non-active connection");
			return true;
		}

		host_requested_sci_mode = event->mode;
		hid_sci_mode_request(event->mode);

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
	    is_ble_peer_sci_conn_rate_event(aeh)) {
		conn_rate_updated(cast_ble_peer_sci_conn_rate_event(aeh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		const struct power_down_event *event =
			cast_power_down_event(aeh);

		if (event->error) {
			return false;
		}

		latency_state |= CONN_IS_POWER_DOWN;

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK) &&
		    !(latency_state & CONN_IS_SCI)) {
			latency_state &= ~CONN_LOW_LATENCY_LOCKED;
			update_llpm_conn_latency_lock();
		} else if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
			   (latency_state & CONN_IS_SCI)) {
			sci_power_event_handle();
		} else {
			/* No handling of the power down event is needed. */
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {

		latency_state &= ~CONN_IS_POWER_DOWN;

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK) &&
		    !(latency_state & CONN_IS_SCI)) {
			latency_state |= CONN_LOW_LATENCY_LOCKED;
			update_llpm_conn_latency_lock();
		} else if (IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE) &&
			   (latency_state & CONN_IS_SCI)) {
			sci_power_event_handle();
		} else {
			/* No handling of the wake up event is needed. */
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
#if CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, hid_sci_mode_request_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_sci_conn_rate_event);
#endif
#if CONFIG_CAF_BLE_SMP_TRANSFER_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, ble_smp_transfer_event);
#endif
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
#ifdef CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
