/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/ble_common_event.h>
#include <caf/events/ble_smp_event.h>

#include "usb_event.h"
#include "config_event.h"

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

#define LIST_FOR_DVFS_STATE(MACRO, ...)						\
	MACRO(INITIALIZING, __VA_ARGS__)					\
	MACRO(LLPM_CONNECTED, __VA_ARGS__)					\
	MACRO(USB_CONNECTED, __VA_ARGS__)					\
	MACRO(CONFIG_CHANNEL, __VA_ARGS__)					\
	MACRO(SMP_TRANSFER, __VA_ARGS__)					\
	MACRO(COUNT, __VA_ARGS__)

#define DVFS_STATE(name, ...) DVFS_STATE_ ## name,

#define GET_ENUM_STRING(name, ...)						\
	IF_ENABLED(CONFIG_DESKTOP_DVFS_STATE_ ## name ## _ENABLE,		\
		(case DVFS_STATE_ ## name: return STRINGIFY(_CONCAT(DVFS_STATE_, name));))

#define _DEFINE_FREQ_MASK_BIT(name, freq_suffix)				\
	IF_ENABLED(CONFIG_DESKTOP_DVFS_STATE_ ## name ## _ ## freq_suffix,	\
	(BIT(DVFS_STATE_ ## name) |))

#define INITIALIZE_DVFS_FREQ_MASK(FREQ_SUFFIX)					\
	LIST_FOR_DVFS_STATE(_DEFINE_FREQ_MASK_BIT, FREQ_SUFFIX) 0

#define INITIALIZE_DVFS_STATE_TIMEOUT(name, ...)				\
	IF_ENABLED(CONFIG_DESKTOP_DVFS_STATE_ ## name ## _ENABLE,		\
	([DVFS_STATE_ ## name] = {						\
		.timeout_ms = CONFIG_DESKTOP_DVFS_STATE_ ## name ## _TIMEOUT_MS,\
	},))

enum dvfs_state {
	LIST_FOR_DVFS_STATE(DVFS_STATE)
};

static const uint8_t dvfs_high_freq_bitmask = INITIALIZE_DVFS_FREQ_MASK(ACTIVE_FREQ_HIGH);
static const uint8_t dvfs_medlow_freq_bitmask = INITIALIZE_DVFS_FREQ_MASK(ACTIVE_FREQ_MEDLOW);

/* Binary mask tracking which states are requested. */
static uint8_t dfvs_requests_state_bitmask;

static enum dvfs_frequency_setting current_freq = DVFS_FREQ_HIGH;

BUILD_ASSERT(sizeof(dvfs_high_freq_bitmask) == sizeof(dfvs_requests_state_bitmask));
BUILD_ASSERT(sizeof(dvfs_medlow_freq_bitmask) == sizeof(dfvs_requests_state_bitmask));
BUILD_ASSERT(CHAR_BIT * sizeof(dfvs_requests_state_bitmask) >= DVFS_STATE_COUNT);

static struct dvfs_retry {
	struct k_work_delayable retry_work;
	uint8_t retries_cnt;
} dvfs_retry;

struct dvfs_state_timeout {
	struct k_work_delayable timeout_work;
	uint16_t timeout_ms;
};

static struct dvfs_state_timeout dvfs_state_timeouts[DVFS_STATE_COUNT] = {
	LIST_FOR_DVFS_STATE(INITIALIZE_DVFS_STATE_TIMEOUT)
};

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
	LIST_FOR_DVFS_STATE(GET_ENUM_STRING);

	default: return "Unknown";
	}
}

static void cancel_dvfs_retry_work(void)
{
	(void) k_work_cancel_delayable(&dvfs_retry.retry_work);
	dvfs_retry.retries_cnt = 0;
}

static void handle_dvfs_error(int32_t err)
{
	if (dvfs_retry.retries_cnt >= DVFS_NUMBER_OF_RETRIES) {
		LOG_ERR("DVFS retry count exceeded.");
		module_set_state(MODULE_STATE_ERROR);
		module_state = STATE_ERROR;
		cancel_dvfs_retry_work();
		return;
	}
	dvfs_retry.retries_cnt++;
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
	(void) k_work_reschedule(&dvfs_retry.retry_work, timeout);
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
	if (dfvs_requests_state_bitmask & dvfs_high_freq_bitmask) {
		return DVFS_FREQ_HIGH;
	} else if (dfvs_requests_state_bitmask & dvfs_medlow_freq_bitmask) {
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
		dfvs_requests_state_bitmask |= BIT(state);
		LOG_DBG("%s ACTIVE", get_dvfs_state_name(state));
	} else {
		dfvs_requests_state_bitmask &= ~BIT(state);
		LOG_DBG("%s NOT ACTIVE", get_dvfs_state_name(state));
	}

	enum dvfs_frequency_setting required_freq = check_required_frequency();

	if ((required_freq != current_freq) &&
	    (!k_work_delayable_is_pending(&dvfs_retry.retry_work))) {
		set_dvfs_freq(required_freq);
	} else if ((required_freq == current_freq) &&
	    (k_work_delayable_is_pending(&dvfs_retry.retry_work))) {
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
static void dvfs_state_timeout_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dvfs_state_timeout *timeout_data = CONTAINER_OF(dwork,
							       struct dvfs_state_timeout,
							       timeout_work);
	enum dvfs_state state = timeout_data - dvfs_state_timeouts;

	__ASSERT_NO_MSG(state < DVFS_STATE_COUNT);
	LOG_DBG("Timeout for %s DVFS state", get_dvfs_state_name(state));
	process_dvfs_states(state, false);
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

			k_work_init_delayable(&dvfs_retry.retry_work,
					      dvfs_retry_work_handler);
			for (size_t i = 0; i < ARRAY_SIZE(dvfs_state_timeouts); i++) {
				k_work_init_delayable(&dvfs_state_timeouts[i].timeout_work,
						      dvfs_state_timeout_work_handler);
			}
			module_state = STATE_READY;

			if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_INITIALIZING_ENABLE)) {
				if (module_flags_check_zero(&req_modules_bm)) {
					process_dvfs_states(DVFS_STATE_INITIALIZING, true);
				}
				get_req_modules(&req_modules_bm);
			} else {
				enum dvfs_frequency_setting required_freq =
								check_required_frequency();
				if (required_freq != current_freq) {
					set_dvfs_freq(required_freq);
				}
			}
		}

		if (!IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_INITIALIZING_ENABLE) ||
		    module_flags_check_zero(&req_modules_bm)) {
			/* Frequency already changed */
			return false;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_INITIALIZING_ENABLE) &&
		    event->state == MODULE_STATE_READY) {
			module_flags_clear_bit(&req_modules_bm, module_idx_get(event->module_id));

			if (module_flags_check_zero(&req_modules_bm)) {
				process_dvfs_states(DVFS_STATE_INITIALIZING, false);
			}
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_LLPM_CONNECTED_ENABLE) &&
	    is_ble_peer_conn_params_event(aeh)) {
		return handle_ble_peer_conn_params_event(cast_ble_peer_conn_params_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_LLPM_CONNECTED_ENABLE) &&
	    is_ble_peer_event(aeh)) {
		if (cast_ble_peer_event(aeh)->state == PEER_STATE_DISCONNECTED) {
			process_dvfs_states(DVFS_STATE_LLPM_CONNECTED, false);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_USB_CONNECTED_ENABLE) &&
	    is_usb_state_event(aeh)) {
		const struct usb_state_event *event = cast_usb_state_event(aeh);

		process_dvfs_states(DVFS_STATE_USB_CONNECTED, event->state == USB_STATE_ACTIVE);

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_SMP_TRANSFER_ENABLE) &&
	    is_ble_smp_transfer_event(aeh)) {
		struct dvfs_state_timeout *smp_transfer_timeout =
			&dvfs_state_timeouts[DVFS_STATE_SMP_TRANSFER];
		if (smp_transfer_timeout->timeout_ms > 0) {
			process_dvfs_states(DVFS_STATE_SMP_TRANSFER, true);
			(void) k_work_reschedule(&smp_transfer_timeout->timeout_work,
						K_MSEC(smp_transfer_timeout->timeout_ms));
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_CONFIG_CHANNEL_ENABLE) && is_config_event(aeh)) {
		struct dvfs_state_timeout *config_channel_timeout =
			&dvfs_state_timeouts[DVFS_STATE_CONFIG_CHANNEL];
		if (config_channel_timeout->timeout_ms > 0) {
			process_dvfs_states(DVFS_STATE_CONFIG_CHANNEL, true);
			(void) k_work_reschedule(&config_channel_timeout->timeout_work,
						K_MSEC(config_channel_timeout->timeout_ms));
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_DVFS_STATE_LLPM_CONNECTED_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
#endif
#if CONFIG_DESKTOP_DVFS_STATE_USB_CONNECTED_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
#endif
#if CONFIG_DESKTOP_DVFS_STATE_SMP_TRANSFER_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, ble_smp_transfer_event);
#endif
#if CONFIG_DESKTOP_DVFS_STATE_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
