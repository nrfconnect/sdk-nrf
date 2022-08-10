/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#include "ei_data_forwarder.h"
#include "ei_data_forwarder_event.h"

#include <caf/events/sensor_event.h>
#include "ml_app_mode_event.h"

#define MODULE ei_data_forwarder
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_EI_DATA_FORWARDER_LOG_LEVEL);

#define EI_DATA_FORWARDER_UART_NODE DT_CHOSEN(ncs_ml_app_ei_data_forwarder_uart)
#define EI_DATA_FORWARDER_UART_DEV DEVICE_DT_GET(EI_DATA_FORWARDER_UART_NODE)

#define UART_BUF_SIZE		CONFIG_ML_APP_EI_DATA_FORWARDER_BUF_SIZE
#define ML_APP_MODE_CONTROL	IS_ENABLED(CONFIG_ML_APP_MODE_EVENTS)

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


static void broadcast_ei_data_forwarder_state(enum ei_data_forwarder_state forwarder_state)
{
	__ASSERT_NO_MSG(forwarder_state != EI_DATA_FORWARDER_STATE_DISABLED);

	struct ei_data_forwarder_event *event = new_ei_data_forwarder_event();

	event->state = forwarder_state;
	APP_EVENT_SUBMIT(event);
}

static void update_state(enum state new_state)
{
	static enum ei_data_forwarder_state forwarder_state = EI_DATA_FORWARDER_STATE_DISABLED;
	enum ei_data_forwarder_state new_forwarder_state;

	switch (new_state) {
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

	state = new_state;

	if (new_forwarder_state != forwarder_state) {
		broadcast_ei_data_forwarder_state(new_forwarder_state);
		forwarder_state = new_forwarder_state;
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

	if (state != STATE_ACTIVE) {
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
		(void)atomic_set(&uart_busy, false);
		LOG_ERR("EI data forwader parsing error: %d", pos);
		report_error();
		return false;
	}

	int err = uart_tx(dev, buf, pos, SYS_FOREVER_MS);

	if (err) {
		(void)atomic_set(&uart_busy, false);
		LOG_ERR("uart_tx error: %d", err);
		report_error();
	}

	return false;
}

static bool handle_ml_app_mode_event(const struct ml_app_mode_event *event)
{
	__ASSERT_NO_MSG(state != STATE_DISABLED);

	if (state == STATE_ERROR) {
		return false;
	}

	if (event->mode == ML_APP_MODE_DATA_FORWARDING) {
		update_state(STATE_ACTIVE);
	} else {
		update_state(STATE_SUSPENDED);
	}

	return false;
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	if (evt->type == UART_TX_DONE) {
		(void)atomic_set(&uart_busy, false);
	}
}

static int init(void)
{
	dev = EI_DATA_FORWARDER_UART_DEV;

	if (!device_is_ready(dev)) {
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
			enum state init_state = ML_APP_MODE_CONTROL ?
						STATE_SUSPENDED : STATE_ACTIVE;

			update_state(init_state);
			module_set_state(MODULE_STATE_READY);
		} else {
			report_error();
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (ML_APP_MODE_CONTROL &&
	    is_ml_app_mode_event(aeh)) {
		return handle_ml_app_mode_event(cast_ml_app_mode_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
#if ML_APP_MODE_CONTROL
APP_EVENT_SUBSCRIBE(MODULE, ml_app_mode_event);
#endif /* ML_APP_MODE_CONTROL */
