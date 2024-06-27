/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/ble_common_event.h>

#include "usb_event.h"

#define MODULE dvfs
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_DVFS_LOG_LEVEL);

#include <ld_dvfs_handler.h>

#define DVFS_RETRY_INITIALIZATION_TIMEOUT	K_MSEC(CONFIG_DESKTOP_DVFS_RETRY_INIT_TIMEOUT_MS)
#define DVFS_RETRY_BUSY_TIMEOUT			K_MSEC(CONFIG_DESKTOP_DVFS_RETRY_BUSY_TIMEOUT_MS)
#define DVFS_NUMBER_OF_RETRIES			CONFIG_DESKTOP_DVFS_RETRY_COUNT
/* Connection intervals used by LLPM are out of Bluetooth LE specification.
 * The intervals are encoded by OR operation with a magic number of 0x0d00.
 */
#define REG_CONN_INTERVAL_LLPM_MASK	0x0d00

enum state {
	STATE_DISABLED,
	STATE_READY,
	STATE_ERROR,
	STATE_COUNT
};

static enum state module_state = STATE_DISABLED;

enum dvfs_state {
	DVFS_STATE_INITIALIZING,
	DVFS_STATE_LLPM_CONNECTED,
	DVFS_STATE_USB_CONNECTED,
	DVFS_STATE_COUNT
};

static const uint8_t dvfs_high_freq_bitmask = BIT(DVFS_STATE_USB_CONNECTED) |
					      BIT(DVFS_STATE_INITIALIZING);
static const uint8_t dvfs_medlow_freq_bitmask = BIT(DVFS_STATE_LLPM_CONNECTED);

/* Binary mask tracking which states are requested.
 * We start with DVFS_STATE_INITIALIZING on as it is active on start.
 */
static uint8_t dfvs_requests_state = BIT(DVFS_STATE_INITIALIZING);

static enum dvfs_frequency_setting current_freq = DVFS_FREQ_HIGH;

static struct dvfs_retry_work_data {
	struct k_work_delayable dvfs_retry_work;
	uint8_t dfvs_retries_cnt;
} dvfs_retry_data;

static const char *get_dvfs_frequency_setting_name(enum dvfs_frequency_setting setting)
{
	switch (setting) {
	case DVFS_FREQ_HIGH: return "DVFS_FREQ_HIGH";
	case DVFS_FREQ_MEDLOW: return "DVFS_FREQ_MEDLOW";
	case DVFS_FREQ_LOW: return "DVFS_FREQ_LOW";
	default: return "Unknown";
	}
}

static const char *get_dvfs_state_name(enum dvfs_state state)
{
	switch (state) {
	case DVFS_STATE_INITIALIZING: return "DVFS_STATE_INITIALIZING";
	case DVFS_STATE_LLPM_CONNECTED: return "DVFS_STATE_LLPM_CONNECTED";
	case DVFS_STATE_USB_CONNECTED: return "DVFS_STATE_USB_CONNECTED";
	default: return "Unknown";
	}
}

static void cancel_dvfs_retry_work(void)
{
	(void) k_work_cancel_delayable(&dvfs_retry_data.dvfs_retry_work);
	dvfs_retry_data.dfvs_retries_cnt = 0;
}

static void handle_dvfs_error(int32_t err)
{
	if (dvfs_retry_data.dfvs_retries_cnt >= DVFS_NUMBER_OF_RETRIES) {
		LOG_ERR("DVFS retry count exceeded.");
		module_set_state(MODULE_STATE_ERROR);
		module_state = STATE_ERROR;
		cancel_dvfs_retry_work();
		return;
	}
	dvfs_retry_data.dfvs_retries_cnt++;
	k_timeout_t timeout;

	if (err == -EBUSY) {
		LOG_DBG("DVFS frequency change in progress");
		timeout = DVFS_RETRY_BUSY_TIMEOUT;
	} else if (err == -EAGAIN) {
		LOG_DBG("DVFS not initialized, trying again.");
		timeout = DVFS_RETRY_INITIALIZATION_TIMEOUT;
	} else {
		LOG_ERR("DVFS freq change returned with error: %d", err);
		module_set_state(MODULE_STATE_ERROR);
		module_state = STATE_ERROR;
		cancel_dvfs_retry_work();
		return;
	}
	(void) k_work_reschedule(&dvfs_retry_data.dvfs_retry_work, timeout);
}

static void set_dvfs_freq(enum dvfs_frequency_setting target_freq)
{
	int32_t ret = dvfs_service_handler_change_freq_setting(target_freq);

	if (ret) {
		handle_dvfs_error(ret);
	} else {
		current_freq = target_freq;
		LOG_INF("Have requested %s frequency",
			get_dvfs_frequency_setting_name(target_freq));
		cancel_dvfs_retry_work();
	}
}

static enum dvfs_frequency_setting check_required_frequency(void)
{
	if (dfvs_requests_state & dvfs_high_freq_bitmask) {
		return DVFS_FREQ_HIGH;
	} else if (dfvs_requests_state & dvfs_medlow_freq_bitmask) {
		return DVFS_FREQ_MEDLOW;
	} else {
		return DVFS_FREQ_LOW;
	}
}

static void process_dvfs_states(enum dvfs_state state, bool turn_on)
{
	if (module_state == STATE_ERROR) {
		return;
	}

	if (turn_on) {
		dfvs_requests_state |= BIT(state);
		LOG_DBG("%s ACTIVE", get_dvfs_state_name(state));
	} else {
		dfvs_requests_state &= ~BIT(state);
		LOG_DBG("%s NOT ACTIVE", get_dvfs_state_name(state));
	}

	enum dvfs_frequency_setting required_freq = check_required_frequency();

	if ((required_freq != current_freq) &&
	    (!k_work_delayable_is_pending(&dvfs_retry_data.dvfs_retry_work))) {
		set_dvfs_freq(required_freq);
	} else if ((required_freq == current_freq) &&
	    (k_work_delayable_is_pending(&dvfs_retry_data.dvfs_retry_work))) {
		cancel_dvfs_retry_work();
	}
}

static bool handle_ble_peer_conn_params_event(const struct ble_peer_conn_params_event *event)
{
	if (!event->updated) {
		/* Ignore the connection parameters update request. */
		return false;
	}

	__ASSERT_NO_MSG(event->interval_min == event->interval_max);

	process_dvfs_states(DVFS_STATE_LLPM_CONNECTED,
			    event->interval_min & REG_CONN_INTERVAL_LLPM_MASK);

	return true;
}

static void dvfs_retry_work_handler(struct k_work *work)
{
	LOG_DBG("Retrying to change DVFS frequency.");

	enum dvfs_frequency_setting required_freq = check_required_frequency();

	__ASSERT_NO_MSG(required_freq != current_freq);

	set_dvfs_freq(required_freq);
}

static void get_req_modules(struct module_flags *mf)
{
	module_flags_set_bit(mf, MODULE_IDX(main));
#if CONFIG_CAF_SETTINGS_LOADER
	module_flags_set_bit(mf, MODULE_IDX(settings_loader));
#endif
#if CONFIG_DESKTOP_BLE_BOND_ENABLE
	module_flags_set_bit(mf, MODULE_IDX(ble_bond));
#endif
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	static struct module_flags req_modules_bm;

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG((dvfs_high_freq_bitmask & dvfs_medlow_freq_bitmask) == 0);
			k_work_init_delayable(&dvfs_retry_data.dvfs_retry_work,
					      dvfs_retry_work_handler);
			get_req_modules(&req_modules_bm);
			module_state = STATE_READY;

			/* In case user implemented empty get_req_modules function */
			if (module_flags_check_zero(&req_modules_bm)) {
				process_dvfs_states(DVFS_STATE_INITIALIZING, false);
				return false;
			}
		}

		if (module_flags_check_zero(&req_modules_bm)) {
			/* Frequency already changed */
			return false;
		}

		if (event->state == MODULE_STATE_READY) {
			module_flags_clear_bit(&req_modules_bm, module_idx_get(event->module_id));

			if (module_flags_check_zero(&req_modules_bm)) {
				process_dvfs_states(DVFS_STATE_INITIALIZING, false);
			}
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_COMMON_EVENTS) && is_ble_peer_conn_params_event(aeh)) {
		return handle_ble_peer_conn_params_event(cast_ble_peer_conn_params_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_COMMON_EVENTS) && is_ble_peer_event(aeh)) {
		if (cast_ble_peer_event(aeh)->state == PEER_STATE_DISCONNECTED) {
			process_dvfs_states(DVFS_STATE_LLPM_CONNECTED, false);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_ENABLE) && is_usb_state_event(aeh)) {
		const struct usb_state_event *event = cast_usb_state_event(aeh);

		process_dvfs_states(DVFS_STATE_USB_CONNECTED, event->state == USB_STATE_ACTIVE);

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_CAF_BLE_COMMON_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
#endif
#if CONFIG_DESKTOP_USB_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
#endif
