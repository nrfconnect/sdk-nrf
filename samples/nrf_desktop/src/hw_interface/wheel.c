/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <assert.h>

#include <zephyr/types.h>

#include <device.h>
#include <sensor.h>

#include "event_manager.h"
#include "wheel_event.h"
#include "power_event.h"

#define MODULE wheel
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_WHEEL_MODULE_LEVEL
#include <logging/sys_log.h>


static struct device *qdec_dev;
static atomic_t active;


static void data_ready_handler(struct device *dev, struct sensor_trigger *trig)
{
	if (!atomic_get(&active)) {
		/* Module is in power down state */
		return;
	}

	struct sensor_value value;

	int err = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &value);
	if (err) {
		SYS_LOG_ERR("cannot get sensor value");
		return;
	}

	struct wheel_event *event = new_wheel_event();
	if (event) {
		s32_t wheel = value.val1;

		if (IS_ENABLED(CONFIG_DESKTOP_WHEEL_INVERT_AXIS)) {
			wheel *= -1;
		}

		static_assert(CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER > 0,
			      "Divider must be non-negative");
		if (CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER > 1) {
			wheel /= CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER;
		}

		event->wheel = max(min(wheel, SCHAR_MAX), SCHAR_MIN);

		EVENT_SUBMIT(event);
	}
}

static void init_fn(void)
{
	qdec_dev = device_get_binding("QDEC");
	if (!qdec_dev) {
		SYS_LOG_ERR("cannot get the device");
		goto error;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ROTATION,
	};

	int err = sensor_trigger_set(qdec_dev, &trig, data_ready_handler);
	if (err) {
		SYS_LOG_ERR("cannot setup trigger");
		goto error;
	}

	module_set_state(MODULE_STATE_READY);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void term_fn(void)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ROTATION,
	};

	int err = sensor_trigger_set(qdec_dev, &trig, NULL);
	if (err) {
		SYS_LOG_ERR("cannot disable trigger");
	}

	module_set_state(MODULE_STATE_OFF);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			init_fn();
			atomic_set(&active, true);

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_get(&active)) {
			init_fn();
			atomic_set(&active, true);
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_get(&active)) {
			atomic_set(&active, false);
			term_fn();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
