/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <bluetooth/services/nus.h>

#include "ei_data_forwarder.h"

#include <caf/events/ble_common_event.h>
#include <caf/events/sensor_event.h>
#include "ml_state_event.h"

#define MODULE ei_data_forwarder
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_EI_DATA_FORWARDER_LOG_LEVEL);

#define MIN_CONN_INT		CONFIG_BT_PERIPHERAL_PREF_MIN_INT
#define MAX_CONN_INT		CONFIG_BT_PERIPHERAL_PREF_MAX_INT

#define DATA_BUF_SIZE		CONFIG_ML_APP_EI_DATA_FORWARDER_BUF_SIZE
#define ML_STATE_CONTROL	IS_ENABLED(CONFIG_ML_APP_ML_STATE_EVENTS)

enum {
	CONN_SECURED			= BIT(0),
	CONN_INTERVAL_VALID		= BIT(1),
};

enum state {
	STATE_DISABLED_ACTIVE,
	STATE_DISABLED_SUSPENDED,
	STATE_ACTIVE,
	STATE_SUSPENDED,
	STATE_BLOCKED,
	STATE_ERROR
};

static uint8_t conn_state;
static struct bt_conn *nus_conn;

static atomic_t nus_busy;
static enum state state = ML_STATE_CONTROL ? STATE_DISABLED_SUSPENDED : STATE_DISABLED_ACTIVE;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static bool is_nus_conn_valid(struct bt_conn *conn, uint8_t conn_state)
{
	if (!conn) {
		return false;
	}

	if (!(conn_state & CONN_INTERVAL_VALID)) {
		return false;
	}

	if (!(conn_state & CONN_SECURED)) {
		return false;
	}

	return true;
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	if ((state != STATE_ACTIVE) || !is_nus_conn_valid(nus_conn, conn_state)) {
		return false;
	}

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) > 0);

	/* Ensure that previous sensor_event was sent. */
	if (!atomic_cas(&nus_busy, false, true)) {
		LOG_WRN("NUS not ready");
		LOG_WRN("Sampling frequency is too high");
		state = STATE_BLOCKED;
		return false;
	}

	static uint8_t buf[DATA_BUF_SIZE];
	int pos = ei_data_forwarder_parse_data(sensor_event_get_data_ptr(event),
					       sensor_event_get_data_cnt(event),
					       buf,
					       sizeof(buf));

	if (pos < 0) {
		atomic_cas(&nus_busy, true, false);
		LOG_ERR("EI data forwader parsing error: %d", pos);
		report_error();
		return false;
	}

	uint32_t mtu = bt_nus_get_mtu(nus_conn);

	if (mtu < pos) {
		atomic_cas(&nus_busy, true, false);
		LOG_WRN("GATT MTU too small: %d > %u", pos, mtu);
		state = STATE_BLOCKED;
	} else {
		int err = bt_nus_send(nus_conn, buf, pos);

		if (err) {
			atomic_cas(&nus_busy, true, false);
			LOG_WRN("bt_nus_tx error: %d", err);
			state = STATE_BLOCKED;
		}
	}

	return false;
}

static bool handle_ml_state_event(const struct ml_state_event *event)
{
	if (state == STATE_ERROR) {
		return false;
	}

	bool disabled = ((state == STATE_DISABLED_ACTIVE) || (state == STATE_DISABLED_SUSPENDED));

	if (event->state == ML_STATE_DATA_FORWARDING) {
		state = disabled ? STATE_DISABLED_ACTIVE : STATE_ACTIVE;
	} else {
		state = disabled ? STATE_DISABLED_SUSPENDED : STATE_SUSPENDED;
	}

	return false;
}

static void bt_nus_sent_cb(struct bt_conn *conn)
{
	atomic_cas(&nus_busy, true, false);
}

static int init_nus(void)
{
	static struct bt_nus_cb nus_cb = {
		.sent = bt_nus_sent_cb,
	};

	int err = bt_nus_init(&nus_cb);

	if (err) {
		LOG_ERR("Cannot initialize NUS (err %d)", err);
	}

	return err;
}

static void init(void)
{
	__ASSERT_NO_MSG((state == STATE_DISABLED_ACTIVE) || (state == STATE_DISABLED_SUSPENDED));

	int err = init_nus();

	if (!err) {
		state = (state == STATE_DISABLED_ACTIVE) ? STATE_ACTIVE : STATE_SUSPENDED;
		module_set_state(MODULE_STATE_READY);
	} else {
		report_error();
	}
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
		init();
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	switch (event->state) {
	case PEER_STATE_CONNECTED:
		atomic_cas(&nus_busy, true, false);
		nus_conn = event->id;
		break;

	case PEER_STATE_SECURED:
		__ASSERT_NO_MSG(nus_conn);
		conn_state |= CONN_SECURED;
		break;

	case PEER_STATE_DISCONNECTED:
	case PEER_STATE_DISCONNECTING:
		conn_state &= ~CONN_SECURED;
		conn_state &= ~CONN_INTERVAL_VALID;
		nus_conn = NULL;
		break;

	default:
		break;
	}

	return false;
}

static bool handle_ble_peer_conn_params_event(const struct ble_peer_conn_params_event *event)
{
	if (!event->updated || (event->id != nus_conn)) {
		return false;
	}

	__ASSERT_NO_MSG(event->interval_min == event->interval_max);

	uint16_t interval = event->interval_min;

	if ((interval >= MIN_CONN_INT) && (interval <= MAX_CONN_INT)) {
		conn_state |= CONN_INTERVAL_VALID;
	} else {
		conn_state &= ~CONN_INTERVAL_VALID;
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_sensor_event(eh)) {
		return handle_sensor_event(cast_sensor_event(eh));
	}

	if (ML_STATE_CONTROL &&
	    is_ml_state_event(eh)) {
		return handle_ml_state_event(cast_ml_state_event(eh));
	}

	if (is_module_state_event(eh)) {
		return handle_module_state_event(cast_module_state_event(eh));
	}

	if (is_ble_peer_event(eh)) {
		return handle_ble_peer_event(cast_ble_peer_event(eh));
	}

	if (is_ble_peer_conn_params_event(eh)) {
		return handle_ble_peer_conn_params_event(cast_ble_peer_conn_params_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, sensor_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
#if ML_STATE_CONTROL
EVENT_SUBSCRIBE(MODULE, ml_state_event);
#endif /* ML_STATE_CONTROL */
