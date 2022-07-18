/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <drivers/sensor_sim.h>

#include "sensor_sim_ctrl_def.h"

#define MODULE sensor_sim_ctrl
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_SENSOR_MANAGER_SENSOR_SIM_CTRL_LOG_LEVEL);

static int set_wave(void)
{
	const struct sim_wave *w = &sim_signal_params.wave;
	int err = sensor_sim_set_wave_param(DEVICE_DT_GET(DT_NODELABEL(sensor_sim)),
					    sim_signal_params.chan,
					    &w->wave_param);

	if (err) {
		LOG_ERR("Cannot set simulated accel params (err %d)", err);
		return err;
	}

	return 0;
}

static void verify_wave_params(void)
{
	const struct sim_wave *w = &sim_signal_params.wave;

	__ASSERT_NO_MSG(w->label != NULL);
	__ASSERT_NO_MSG(w->wave_param.type < WAVE_GEN_TYPE_COUNT);
	__ASSERT_NO_MSG((w->wave_param.period_ms > 0) ||
			(w->wave_param.type == WAVE_GEN_TYPE_NONE));
	ARG_UNUSED(w);
}

static int init(void)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		verify_wave_params();
	}
	return set_wave();

}

static bool event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			if (!init()) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
