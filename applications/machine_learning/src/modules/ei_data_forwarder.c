/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>

#include <caf/events/sensor_event.h>
#include "ml_state_event.h"

#define MODULE ei_data_forwarder
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_EI_DATA_FORWARDER_LOG_LEVEL);

#define UART_LABEL		CONFIG_ML_APP_EI_DATA_FORWARDER_UART_DEV
#define UART_BUF_SIZE		CONFIG_ML_APP_EI_DATA_FORWARDER_BUF_SIZE

#define ML_STATE_CONTROL	IS_ENABLED(CONFIG_ML_APP_ML_STATE_EVENTS)

static const struct device *dev;
static atomic_t uart_busy;

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_SUSPENDED,
	STATE_ERROR
};

static enum state state;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static int snprintf_error_check(int res, size_t buf_size)
{
	if ((res < 0) || (res >= buf_size)) {
		LOG_ERR("snprintf returned %d", res);
		report_error();
		return res;
	}

	return 0;
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	if (state != STATE_ACTIVE) {
		return false;
	}

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) > 0);

	/* Ensure that previous sensor_event was sent. */
	if (!atomic_cas(&uart_busy, false, true)) {
		LOG_ERR("UART not ready");
		LOG_ERR("Sampling frequency is too high");
		report_error();
		return false;
	}

	static uint8_t buf[UART_BUF_SIZE];
	const float *data_ptr = sensor_event_get_data_ptr(event);
	size_t data_cnt = sensor_event_get_data_cnt(event);
	int pos = 0;
	int tmp;

	for (size_t i = 0; i < 2 * data_cnt; i++) {
		if ((i % 2) == 0) {
			tmp = snprintf(&buf[pos], sizeof(buf) - pos, "%.2f", data_ptr[i / 2]);
		} else if (i == (2 * data_cnt - 1)) {
			tmp = snprintf(&buf[pos], sizeof(buf) - pos, "\r\n");
		} else {
			tmp = snprintf(&buf[pos], sizeof(buf) - pos, ",");
		}

		if (snprintf_error_check(tmp, sizeof(buf) - pos)) {
			return false;
		}
		pos += tmp;
	}

	int err = uart_tx(dev, buf, pos, SYS_FOREVER_MS);

	if (err) {
		LOG_ERR("uart_tx error: %d", err);
		report_error();
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
		state = STATE_ACTIVE;
	} else {
		state = STATE_SUSPENDED;
	}

	return false;
}

static void uart_cb(const struct device *dev, struct uart_event *evt,
		    void *user_data)
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

static bool event_handler(const struct event_header *eh)
{
	if (is_sensor_event(eh)) {
		return handle_sensor_event(cast_sensor_event(eh));
	}

	if (ML_STATE_CONTROL && is_ml_state_event(eh)) {
		return handle_ml_state_event(cast_ml_state_event(eh));
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);

			if (!init()) {
				state = ML_STATE_CONTROL ? STATE_SUSPENDED : STATE_ACTIVE;
				module_set_state(MODULE_STATE_READY);
			} else {
				report_error();
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if ML_STATE_CONTROL
EVENT_SUBSCRIBE(MODULE, ml_state_event);
#endif /* ML_STATE_CONTROL */
EVENT_SUBSCRIBE(MODULE, sensor_event);
