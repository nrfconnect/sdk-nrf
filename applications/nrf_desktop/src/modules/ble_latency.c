/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>

#include "ble_event.h"

#define MODULE ble_latency
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_LATENCY_LOG_LEVEL);

static struct k_delayed_work security_timeout;
static struct bt_conn *active_conn;

#define SECURITY_FAIL_TIMEOUT_MS \
	K_SECONDS(CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S)


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

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&security_timeout,
					    security_timeout_fn);
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
			/* Fall through. */
		case PEER_STATE_SECURED:
			k_delayed_work_cancel(&security_timeout);
			break;

		default:
			/* Ignore. */
			break;
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
