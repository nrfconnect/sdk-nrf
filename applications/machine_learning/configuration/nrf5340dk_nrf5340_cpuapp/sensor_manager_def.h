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


static const struct sm_sampled_channel accel_chan[] = {
	{
		.chan = SENSOR_CHAN_ACCEL_X,
		.data_cnt = 1,
	},
	{
		.chan = SENSOR_CHAN_ACCEL_Y,
		.data_cnt = 1,
	},
	{
		.chan = SENSOR_CHAN_ACCEL_Z,
		.data_cnt = 1,
	},
};

static const struct sm_sensor_config sensor_configs[] = {
	{
		.dev_name = "SENSOR_SIM",
		.event_descr = CONFIG_ML_APP_SENSOR_EVENT_DESCR,
		.chans = accel_chan,
		.chan_cnt = ARRAY_SIZE(accel_chan),
		.sampling_period_ms = 20,
		.active_events_limit = 3,
	},
};
