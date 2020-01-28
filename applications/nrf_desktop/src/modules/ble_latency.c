/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>

#include "ble_event.h"
#include "config_event.h"

#define MODULE ble_latency
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_LATENCY_LOG_LEVEL);

#define SECURITY_FAIL_TIMEOUT_MS \
	K_SECONDS(CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S)
#define LOW_LATENCY_CHECK_PERIOD_MS	5000
#define DEFAULT_LATENCY			CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY

static struct bt_conn *active_conn;
static struct k_delayed_work security_timeout;
static struct k_delayed_work low_latency_check;

enum {
	LOW_LATENCY_ENABLED		= BIT(0),
	LOW_LATENCY_REQUIRED		= BIT(1),
};

static u8_t latency_state;


static void security_timeout_fn(struct k_work *w)
{
	/* Assert one local identity holds exactly one bond.
	 * One local identity is unused.
	 */
	BUILD_ASSERT(CONFIG_BT_MAX_PAIRED == (CONFIG_BT_ID_MAX - 1));
	BUILD_ASSERT(CONFIG_BT_MAX_CONN == 1);
	BUILD_ASSERT(SECURITY_FAIL_TIMEOUT_MS > 0);
	__ASSERT_NO_MSG(active_conn);

	int err = bt_conn_disconnect(active_conn,
				     BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err == -ENOTCONN) {
		err = 0;
	}
	LOG_WRN("Security establishment failed - device %s",
		err ? "failed to disconnect" : "disconnected");
}

static void set_ble_latency(bool low_latency)
{
	struct bt_conn_info info;

	int err = bt_conn_get_info(active_conn, &info);

	if (err) {
		LOG_WRN("Cannot get conn info (%d)", err);
		return;
	}

	__ASSERT_NO_MSG(info.role == BT_CONN_ROLE_SLAVE);

	const struct bt_le_conn_param param = {
		.interval_min = info.le.interval,
		.interval_max = info.le.interval,
		.latency = (low_latency) ? (0) : (DEFAULT_LATENCY),
		.timeout = info.le.timeout
	};

	err = bt_conn_le_param_update(active_conn, &param);

	if (!err || (err == -EALREADY)) {
		LOG_INF("BLE latency %screased", low_latency ? "de" : "in");

		if (low_latency) {
			latency_state |= LOW_LATENCY_ENABLED;
		} else {
			latency_state &= ~LOW_LATENCY_ENABLED;
		}

		if (err == -EALREADY) {
			LOG_INF("Conn parameters were already updated");
		}
	} else if (err == -EINVAL) {
		LOG_INF("LLPM conn parameters - do not update");
	} else {
		LOG_WRN("Failed to update conn parameters (err %d)", err);
	}
}

static void low_latency_check_fn(struct k_work *w)
{
	if (latency_state & LOW_LATENCY_REQUIRED) {
		latency_state &= ~LOW_LATENCY_REQUIRED;
		k_delayed_work_submit(&low_latency_check,
				      LOW_LATENCY_CHECK_PERIOD_MS);
	} else {
		LOG_INF("Low latency timed out");
		set_ble_latency(false);
	}
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	if (latency == 0) {
		latency_state |= LOW_LATENCY_ENABLED;
		k_delayed_work_submit(&low_latency_check,
				      LOW_LATENCY_CHECK_PERIOD_MS);
	} else {
		latency_state &= ~LOW_LATENCY_ENABLED;
		k_delayed_work_cancel(&low_latency_check);
	}
}

static void init(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.le_param_updated = le_param_updated,
	};

	bt_conn_cb_register(&conn_callbacks);

	k_delayed_work_init(&security_timeout, security_timeout_fn);
	k_delayed_work_init(&low_latency_check, low_latency_check_fn);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			init();
			__ASSERT_NO_MSG(!initialized);
			initialized = true;
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
			active_conn = event->id;
			k_delayed_work_submit(&security_timeout,
					      SECURITY_FAIL_TIMEOUT_MS);
			break;

		case PEER_STATE_DISCONNECTED:
			__ASSERT_NO_MSG(active_conn == event->id);
			active_conn = NULL;

			/* Clear BLE latency state. */
			latency_state = 0;
			k_delayed_work_cancel(&low_latency_check);

			/* Fall-through */

		case PEER_STATE_SECURED:
			k_delayed_work_cancel(&security_timeout);
			break;

		default:
			/* Ignore. */
			break;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    (is_config_event(eh) || is_config_fetch_request_event(eh))) {
		if (!active_conn) {
			return false;
		}

		if (latency_state & LOW_LATENCY_ENABLED) {
			latency_state |= LOW_LATENCY_REQUIRED;
		} else {
			set_ble_latency(true);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
#endif
