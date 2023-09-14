/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <bluetooth/services/nsms.h>

#include <caf/events/sensor_event.h>
#include <caf/events/power_event.h>

#include "app_sensor_event.h"

#define MODULE nsms
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);


#define BUF_SIZE 64

BT_NSMS_DEF(nsms_imu, "IMU", false, "Unknown", BUF_SIZE);
BT_NSMS_DEF(nsms_env, "Environmental", false, "Unknown", BUF_SIZE);

static void init(void)
{
	module_set_state(MODULE_STATE_READY);
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	size_t data_cnt = sensor_event_get_data_cnt(event);
	float float_data[data_cnt];
	const struct sensor_value *data_ptr = sensor_event_get_data_ptr(event);

	char buf[BUF_SIZE];

	int len = sprintf(buf, "%s ", event->descr);

	for (size_t i = 0; i < data_cnt; i++) {
		float_data[i] = sensor_value_to_double(&data_ptr[i]);

		len += sprintf(buf + len, "%.3f ", (double)float_data[i]);
	}

	LOG_INF("%s", buf);

	if (!strcmp(event->descr, "env")) {
		bt_nsms_set_status(&nsms_env, buf);
	} else if (!strcmp(event->descr, "imu")) {
		bt_nsms_set_status(&nsms_imu, buf);
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
