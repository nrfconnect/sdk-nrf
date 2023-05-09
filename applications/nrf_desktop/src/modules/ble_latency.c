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
#include <caf/events/power_event.h>

#define MODULE ble_latency
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_LATENCY_LOG_LEVEL);

#define SECURITY_FAIL_TIMEOUT_MS \
	K_SECONDS(CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S)
#define LOW_LATENCY_CHECK_PERIOD_MS	K_SECONDS(5)
#define DEFAULT_LATENCY			CONFIG_BT_PERIPHERAL_PREF_LATENCY
#define DEFAULT_TIMEOUT			CONFIG_BT_PERIPHERAL_PREF_TIMEOUT
#define REG_CONN_INTERVAL_LLPM_MASK	0x0d00
#define REG_CONN_INTERVAL_BLE_DEFAULT	0x0006

static struct bt_conn *active_conn;
static struct k_work_delayable security_timeout;
static struct k_work_delayable low_latency_check;

enum {
	CONN_LOW_LATENCY_ENABLED	= BIT(0),
	CONN_LOW_LATENCY_REQUIRED	= BIT(1),
	CONN_LOW_LATENCY_LOCKED		= BIT(2),
	CONN_IS_LLPM			= BIT(3),
	CONN_IS_SECURED			= BIT(4),
};

static uint8_t latency_state;


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

	if (!err || (err == -EALREADY)) {
		LOG_INF("Init connection parameters are set");
	} else {
		LOG_WRN("Failed to update conn parameters (err %d)", err);
	}
}

static void set_conn_latency(bool low_latency)
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
	uint16_t interval = (info.le.interval & REG_CONN_INTERVAL_LLPM_MASK) ?
			  REG_CONN_INTERVAL_BLE_DEFAULT : info.le.interval;
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

static void conn_params_updated(const struct ble_peer_conn_params_event *event)
{
	if (!event->updated) {
		/* Ignore the connection parameters update request. */
		return;
	}

	__ASSERT_NO_MSG(event->interval_min == event->interval_max);

	if (event->interval_min & REG_CONN_INTERVAL_LLPM_MASK) {
		latency_state |= CONN_IS_LLPM;
	} else {
		latency_state &= ~CONN_IS_LLPM;
	}

	if (event->latency == 0) {
		latency_state |= CONN_LOW_LATENCY_ENABLED;
		latency_state &= ~CONN_LOW_LATENCY_REQUIRED;
		k_work_reschedule(&low_latency_check,
				      LOW_LATENCY_CHECK_PERIOD_MS);
	} else {
		latency_state &= ~CONN_LOW_LATENCY_ENABLED;
		/* Cancel cannot fail if executed from another work's context. */
		(void)k_work_cancel_delayable(&low_latency_check);
	}

	update_llpm_conn_latency_lock();
}

static void init(void)
{
	k_work_init_delayable(&security_timeout, security_timeout_fn);
	k_work_init_delayable(&low_latency_check, low_latency_check_fn);
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
			set_init_conn_params();
			k_work_reschedule(&security_timeout,
					      SECURITY_FAIL_TIMEOUT_MS);
			break;

		case PEER_STATE_DISCONNECTED:
			__ASSERT_NO_MSG(active_conn == event->id);
			active_conn = NULL;

			/* Clear BLE latency state. */
			latency_state = 0;
			/* Cancel cannot fail if executed from another work's context. */
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

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK) &&
	    IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		const struct power_down_event *event =
			cast_power_down_event(aeh);

		if (!event->error) {
			latency_state &= ~CONN_LOW_LATENCY_LOCKED;
			update_llpm_conn_latency_lock();
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK) &&
	    IS_ENABLED(CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
		latency_state |= CONN_LOW_LATENCY_LOCKED;
		update_llpm_conn_latency_lock();

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
#if CONFIG_CAF_BLE_SMP_TRANSFER_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, ble_smp_transfer_event);
#endif
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
#if CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK && CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
