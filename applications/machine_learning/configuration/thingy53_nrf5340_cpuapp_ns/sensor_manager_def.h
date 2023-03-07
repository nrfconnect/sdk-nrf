/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/sensor_manager.h>

/* This configuration file is included only once from sensor_manager module and holds
 * information about the sampled sensors.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} sensor_manager_def_include_once;

static struct sm_trigger sensor_trigger = {
	.cfg = {
		.type = SENSOR_TRIG_MOTION,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	},
	.activation = {
		.type = ACT_TYPE_ABS,
		.thresh = FLOAT_TO_SENSOR_VALUE (CONFIG_ADXL362_ACTIVITY_THRESHOLD *
			(IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_8G) ? 8.0 :
			 IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_4G) ? 4.0 :
			 IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_2G) ? 2.0 : 1/0)
			 / 2048.0),
		.timeout_ms = (CONFIG_APP_SENSOR_SLEEP_TO) * 1000
	}
};

static const struct caf_sampled_channel accel_chan[] = {
	{
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.data_cnt = 3,
	},
};

static const struct sm_sensor_config sensor_configs[] = {
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(adxl362)),
		.event_descr = CONFIG_ML_APP_SENSOR_EVENT_DESCR,
		.chans = accel_chan,
		.chan_cnt = ARRAY_SIZE(accel_chan),
		.sampling_period_ms = 20,
		.active_events_limit = 3,
		.trigger = &sensor_trigger,
	},
};
