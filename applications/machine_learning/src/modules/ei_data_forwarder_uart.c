/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/uart.h>

#include "ei_data_forwarder.h"
#include "ei_data_forwarder_event.h"

#include <caf/events/sensor_event.h>
#include "ml_state_event.h"

#define MODULE ei_data_forwarder
#include <caf/events/module_state_event.h>
#include <caf/events/sensor_force_active_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_EI_DATA_FORWARDER_LOG_LEVEL);

#define UART_LABEL		CONFIG_ML_APP_EI_DATA_FORWARDER_UART_DEV
#define UART_BUF_SIZE		CONFIG_ML_APP_EI_DATA_FORWARDER_BUF_SIZE
#define ML_STATE_CONTROL	IS_ENABLED(CONFIG_ML_APP_ML_STATE_EVENTS)

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_SUSPENDED,
	STATE_BLOCKED,
	STATE_ERROR
};

BUILD_ASSERT(ARRAY_SIZE(CONFIG_ML_APP_EI_DATA_FORWARDER_SENSOR_EVENT_DESCR) > 1);
static const char *handled_sensor_event_descr = CONFIG_ML_APP_EI_DATA_FORWARDER_SENSOR_EVENT_DESCR;

static const struct device *dev;
static atomic_t uart_busy;
static enum state state = STATE_DISABLED;
static bool sensor_active;


static void broadcast_ei_data_forwarder_state(enum ei_data_forwarder_state forwarder_state)
{
	__ASSERT_NO_MSG(forwarder_state != EI_DATA_FORWARDER_STATE_DISABLED);

	struct ei_data_forwarder_event *event = new_ei_data_forwarder_event();

	event->state = forwarder_state;
	EVENT_SUBMIT(event);
}

static void update_state(enum state new_state)
{
	static enum ei_data_forwarder_state forwarder_state = EI_DATA_FORWARDER_STATE_DISABLED;
	enum ei_data_forwarder_state new_forwarder_state;
	bool force_broadcast = false;

	switch (new_state) {
	case STATE_DISABLED:
		/* Broadcast the previous state */
		new_forwarder_state = forwarder_state;
		force_broadcast = true;
		break;

	case STATE_ACTIVE:
		new_forwarder_state = EI_DATA_FORWARDER_STATE_TRANSMITTING;
		break;

	case STATE_SUSPENDED:
	case STATE_BLOCKED:
		new_forwarder_state = EI_DATA_FORWARDER_STATE_CONNECTED;
		break;

	case STATE_ERROR:
	default:
		new_forwarder_state = EI_DATA_FORWARDER_STATE_DISCONNECTED;
		break;
	}

	if (new_state != STATE_DISABLED)
		state = new_state;

	if ((new_forwarder_state != forwarder_state) || force_broadcast) {
		if (sensor_active) {
			broadcast_ei_data_forwarder_state(new_forwarder_state);
		}
		forwarder_state = new_forwarder_state;

		if (IS_ENABLED(CONFIG_CAF_SENSOR_FORCE_ACTIVE_EVENTS)) {
			sensor_force_active(
				handled_sensor_event_descr,
				forwarder_state == EI_DATA_FORWARDER_STATE_TRANSMITTING);
		}
	}
}

static void report_error(void)
{
	update_state(STATE_ERROR);
	module_set_state(MODULE_STATE_ERROR);
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	if ((event->descr != handled_sensor_event_descr) &&
	    strcmp(event->descr, handled_sensor_event_descr)) {
		return false;
	}

	if ((state != STATE_ACTIVE) || !sensor_active) {
		return false;
	}

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) > 0);

	/* Ensure that previous sensor_event was sent. */
	if (!atomic_cas(&uart_busy, false, true)) {
		LOG_WRN("UART not ready");
		LOG_WRN("Sampling frequency is too high");
		update_state(STATE_BLOCKED);
		return false;
	}

	static uint8_t buf[UART_BUF_SIZE];

	int pos = ei_data_forwarder_parse_data(sensor_event_get_data_ptr(event),
					       sensor_event_get_data_cnt(event),
					       (char *)buf,
					       sizeof(buf));

	if (pos < 0) {
		atomic_cas(&uart_busy, true, false);
		LOG_ERR("EI data forwader parsing error: %d", pos);
		report_error();
		return false;
	}

	int err = uart_tx(dev, buf, pos, SYS_FOREVER_MS);

	if (err) {
		atomic_cas(&uart_busy, true, false);
		LOG_ERR("uart_tx error: %d", err);
		report_error();
	}

	return false;
}

static bool handle_sensor_state_event(const struct sensor_state_event *event)
{
	if ((event->descr != handled_sensor_event_descr) &&
	    strcmp(event->descr, handled_sensor_event_descr)) {
		return false;
	}

	bool new_sensor_state = event->state == SENSOR_STATE_ACTIVE;

	if (new_sensor_state != sensor_active) {
		sensor_active = new_sensor_state;
		if (sensor_active) {
			module_set_state(MODULE_STATE_READY);
			update_state(STATE_DISABLED);
		} else {
			module_set_state(MODULE_STATE_OFF);
		}
	}

	return false;
}

static bool handle_ml_state_event(const struct ml_state_event *event)
{
	__ASSERT_NO_MSG(state != STATE_DISABLED);

	if (state == STATE_ERROR) {
		return false;
	}

	if (event->state == ML_STATE_DATA_FORWARDING) {
		update_state(STATE_ACTIVE);
	} else {
		update_state(STATE_SUSPENDED);
	}

	return false;
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	if (evt->type == UART_TX_DONE) {
		atomic_cas(&uart_busy, true, false);
	}
}

static int init(void)
{
	dev = device_get_binding(UART_LABEL);

	if (!dev) {
		LOG_ERR("UART device binding failed");
		return -ENXIO;
	}

	int err = uart_callback_set(dev, uart_cb, NULL);

	if (err) {
		LOG_ERR("Cannot set UART callback (err %d)", err);
	}

	return err;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		__ASSERT_NO_MSG(state == STATE_DISABLED);

		int err = init();

		if (!err) {
			enum state init_state = ML_STATE_CONTROL ? STATE_SUSPENDED : STATE_ACTIVE;

			update_state(init_state);
			module_set_state(MODULE_STATE_READY);
		} else {
			report_error();
		}
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_sensor_event(eh)) {
		return handle_sensor_event(cast_sensor_event(eh));
	}
	if (is_sensor_state_event(eh)) {
		return handle_sensor_state_event(cast_sensor_state_event(eh));
	}

	if (ML_STATE_CONTROL &&
	    is_ml_state_event(eh)) {
		return handle_ml_state_event(cast_ml_state_event(eh));
	}

	if (is_module_state_event(eh)) {
		return handle_module_state_event(cast_module_state_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, sensor_event);
EVENT_SUBSCRIBE(MODULE, sensor_state_event);
#if ML_STATE_CONTROL
EVENT_SUBSCRIBE(MODULE, ml_state_event);
#endif /* ML_STATE_CONTROL */
