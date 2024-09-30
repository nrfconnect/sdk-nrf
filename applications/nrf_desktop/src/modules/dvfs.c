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

#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/devicetree.h>

#define CLOCK_NODE DT_ALIAS(nrfdesktop_dvfs_clock)

#if !DT_NODE_HAS_STATUS(CLOCK_NODE, okay)
#error "Alias 'nrfdesktop-dvfs-clock' is not defined in the device tree!"
#endif

static const struct device *dvfs_clock_dev = DEVICE_DT_GET(CLOCK_NODE);
static struct onoff_client cli;

/* The used nrf_clock_control driver implementation does not support
 * clock precision and clock accuracy ppm.
 */
#define DESKTOP_DVFS_CLOCK_PRECISION	0
#define DESKTOP_DVFS_CLOCK_ACCURACY_PPM	0

static struct nrf_clock_spec spec = {
	.accuracy = DESKTOP_DVFS_CLOCK_ACCURACY_PPM,
	.precision = DESKTOP_DVFS_CLOCK_PRECISION,
};

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

struct dvfs_frequency {
	uint32_t freq;
	uint8_t bitmask;
};

static const struct dvfs_frequency dvfs_freq_array[] = {
	{
		.freq = CONFIG_DESKTOP_DVFS_FREQ_HIGH,
		.bitmask = INITIALIZE_DVFS_FREQ_MASK(ACTIVE_FREQ_HIGH)
	},
	{
		.freq = CONFIG_DESKTOP_DVFS_FREQ_MED,
		.bitmask = INITIALIZE_DVFS_FREQ_MASK(ACTIVE_FREQ_MEDLOW)
	},
	{
		.freq = CONFIG_DESKTOP_DVFS_FREQ_LOW,
		.bitmask = UINT8_MAX
	},
};

/* Binary mask tracking which states are requested. */
static uint8_t dfvs_requests_state_bitmask;

/* SoC starts with 320MHz frequency */
static uint32_t current_freq = dvfs_freq_array[0].freq;

BUILD_ASSERT(sizeof(dvfs_freq_array[0].bitmask) == sizeof(dfvs_requests_state_bitmask));
BUILD_ASSERT(CHAR_BIT * sizeof(dfvs_requests_state_bitmask) >= DVFS_STATE_COUNT);

static struct dvfs_retry {
	struct k_work_delayable retry_work;
	uint8_t retries_cnt;
} dvfs_retry;

struct dvfs_state_timeout {
	struct k_work_delayable timeout_work;
	uint16_t timeout_ms;
};

static struct k_work dvfs_notify_work;
static uint32_t requested_freq;
static bool request_in_progress;

static struct dvfs_state_timeout dvfs_state_timeouts[DVFS_STATE_COUNT] = {
	LIST_FOR_DVFS_STATE(INITIALIZE_DVFS_STATE_TIMEOUT)
};

static const char *get_dvfs_state_name(enum dvfs_state state)
{
	switch (state) {
	LIST_FOR_DVFS_STATE(GET_ENUM_STRING);

	default: return "Unknown";
	}
}

static void dvfs_fatal_error(void)
{
	module_set_state(MODULE_STATE_ERROR);
	module_state = STATE_ERROR;
	(void) k_work_cancel_delayable(&dvfs_retry.retry_work);
	for (size_t i = 0; i < ARRAY_SIZE(dvfs_state_timeouts); i++) {
		(void) k_work_cancel_delayable(&dvfs_state_timeouts[i].timeout_work);
	}
}

static void handle_dvfs_error(int32_t err)
{
	if (dvfs_retry.retries_cnt >= DVFS_NUMBER_OF_RETRIES) {
		LOG_ERR("DVFS retry count exceeded.");
		dvfs_fatal_error();
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
		dvfs_fatal_error();
		return;
	}
	(void) k_work_reschedule(&dvfs_retry.retry_work, timeout);
}

static uint32_t check_required_frequency(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(dvfs_freq_array) - 1; i++) {
		if (dfvs_requests_state_bitmask & dvfs_freq_array[i].bitmask) {
			return dvfs_freq_array[i].freq;
		}
	}

	/* If no state is active, return the lowest frequency. */
	return dvfs_freq_array[ARRAY_SIZE(dvfs_freq_array) - 1].freq;
}

static void dvfs_notify_cb(struct onoff_manager *srv,
			   struct onoff_client *cli,
			   uint32_t state,
			   int res)
{
	(void) k_work_submit(&dvfs_notify_work);
}

static void set_dvfs_freq(uint32_t target_freq)
{
	int ret;

	if (spec.frequency != 0) {
		ret = nrf_clock_control_release(dvfs_clock_dev, &spec);
		if (ret < 0) {
			LOG_ERR("Failed to release requested clock specs, error: %d", ret);
			dvfs_fatal_error();
			return;
		}
	}

	sys_notify_init_callback(&cli.notify, dvfs_notify_cb);

	spec.frequency = target_freq;
	ret = nrf_clock_control_request(dvfs_clock_dev, &spec, &cli);
	if (ret) {
		handle_dvfs_error(ret);
	} else {
		LOG_INF("Have requested %" PRIu32 " frequency", target_freq);
		requested_freq = target_freq;
		request_in_progress = true;
	}
}

static void dvfs_frequency_update(void)
{
	uint32_t required_freq = check_required_frequency();

	if ((!k_work_delayable_is_pending(&dvfs_retry.retry_work)) &&
	    !request_in_progress && (required_freq != current_freq)) {
		set_dvfs_freq(required_freq);
	} else if ((required_freq == current_freq) &&
		   (k_work_delayable_is_pending(&dvfs_retry.retry_work))) {
		(void) k_work_cancel_delayable(&dvfs_retry.retry_work);
		/* retry_cnt should be only cleared on successful frequency change. */
	}
}

static void dvfs_notify_work_handler(struct k_work *work)
{
	int res;
	int ret = sys_notify_fetch_result(&cli.notify, &res);

	if (ret < 0) {
		LOG_ERR("Work not completed, verify usage of notify API");
		dvfs_fatal_error();
		return;
	}

	__ASSERT_NO_MSG(request_in_progress);
	request_in_progress = false;

	if (res < 0) {
		handle_dvfs_error(res);
		return;
	}
	ret = clock_control_get_rate(dvfs_clock_dev, NULL, &current_freq);
	if (ret < 0) {
		LOG_ERR("Failed to get current frequency with error: %d", ret);
		dvfs_fatal_error();
		return;
	}
	if (requested_freq != current_freq) {
		/*
		 * In current solution it is assumed that no other module
		 * will change cpu frequency.
		 */
		LOG_ERR("Requested frequency %" PRIu32
			" is not the same as current frequency %" PRIu32,
			requested_freq, current_freq);
		dvfs_fatal_error();
		return;
	}

	dvfs_retry.retries_cnt = 0;
	LOG_INF("DVFS completed, current frequency is: %" PRIu32, current_freq);

	/* Check if there were any new requests. */
	dvfs_frequency_update();
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

	dvfs_frequency_update();
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

	uint32_t required_freq = check_required_frequency();

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
			BUILD_ASSERT(ARRAY_SIZE(dvfs_freq_array) == 3,
				"Add asserts if frequency array is extended");
			__ASSERT_NO_MSG((dvfs_freq_array[0].bitmask &
					 dvfs_freq_array[1].bitmask) == 0);

			k_work_init(&dvfs_notify_work, dvfs_notify_work_handler);
			k_work_init_delayable(&dvfs_retry.retry_work, dvfs_retry_work_handler);
			for (size_t i = 0; i < ARRAY_SIZE(dvfs_state_timeouts); i++) {
				k_work_init_delayable(&dvfs_state_timeouts[i].timeout_work,
						      dvfs_state_timeout_work_handler);
			}
			int ret = clock_control_get_rate(dvfs_clock_dev,
					NULL, &current_freq);

			if (ret < 0) {
				LOG_ERR("Failed to get current frequency with error: %d",
					ret);
				dvfs_fatal_error();
				return false;
			}
			module_state = STATE_READY;

			if (IS_ENABLED(CONFIG_DESKTOP_DVFS_STATE_INITIALIZING_ENABLE)) {
				get_req_modules(&req_modules_bm);
				if (!module_flags_check_zero(&req_modules_bm)) {
					process_dvfs_states(DVFS_STATE_INITIALIZING, true);
				} else {
					dvfs_frequency_update();
				}
			} else {
				dvfs_frequency_update();
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
