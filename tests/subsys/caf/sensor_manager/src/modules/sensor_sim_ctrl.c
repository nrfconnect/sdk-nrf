/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <drivers/sensor_sim.h>

#include "sensor_sim_ctrl_def.h"
#include "test_events.h"
#include <zephyr/ztest.h>


static int set_wave(void)
{
	const struct sim_wave *w = &sim_signal_params.wave;
	int err = sensor_sim_set_wave_param(DEVICE_DT_GET(DT_NODELABEL(sensor_sim_1)),
					    sim_signal_params.chan,
					    &w->wave_param);

	if (err) {
		zassert_ok(err, "Cannot set simulated accel params ");
		return err;
	}

	err = sensor_sim_set_wave_param(DEVICE_DT_GET(DT_NODELABEL(sensor_sim_2)),
					    sim_signal_params.chan,
					    &w->wave_param);

	if (err) {
		zassert_ok(err, "Cannot set simulated accel params ");
		return err;
	}

	err = sensor_sim_set_wave_param(DEVICE_DT_GET(DT_NODELABEL(sensor_sim_3)),
					    sim_signal_params.chan,
					    &w->wave_param);

	if (err) {
		zassert_ok(err, "Cannot set simulated accel params ");
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
	if (is_test_start_event(aeh)) {
		struct test_start_event *st = cast_test_start_event(aeh);

		if (st->test_id == TEST_SENSOR_INIT) {
			zassert_ok(init(), "Error when initializing sensor");
			struct test_end_event *et = new_test_end_event();

			zassert_not_null(et, "Failed to allocate event");
			et->test_id = st->test_id;
			APP_EVENT_SUBMIT(et);

		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, test_start_event);
