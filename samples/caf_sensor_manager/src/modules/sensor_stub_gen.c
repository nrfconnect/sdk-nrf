/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <drivers/sensor_stub.h>

#define MODULE sensor_stub_gen
#include <caf/events/module_state_event.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_SENSOR_STUB_GEN_LOG_LEVEL);


#define SENSOR_FRACTIONAL_MAX (1000000)
#define SENSOR_STUB_GEN_X_STEP (0.1)
#define SENSOR_STUB_GEN_X_MAX  (5.0)
#define SENSOR_STUB_GEN_X_MIN  (-5.0)

#define SENSOR_STUB_GEN_Y_STEP (0.1)
#define SENSOR_STUB_GEN_Y_MAX  (2.0)
#define SENSOR_STUB_GEN_Y_MIN  (-2.0)

#define SENSOR_STUB_GEN_Z_STEP (0.2)
#define SENSOR_STUB_GEN_Z_MAX  (1.0)
#define SENSOR_STUB_GEN_Z_MIN  (0.0)

#define SENSOR_STUB_SENSOR_FP(x) ((int64_t)((x) * SENSOR_FRACTIONAL_MAX))

enum {
	SENSOR_STUB_CH_X,
	SENSOR_STUB_CH_Y,
	SENSOR_STUB_CH_Z,
	SENSOR_STUB_CH_CNT,
};

/* All the data for implemented sensor simulation */
struct sensor_stub_gen_ch_data {
	/* Current value */
	struct sensor_value val;
	/* Current direction of the changes */
	bool dir;
};

/* Parameters of the signal generated on the selected channel */
struct sensor_stub_gen_ch_params {
	int64_t step, max, min;
};

/* SENSOR_CHAN_ACCEL_X, SENSOR_CHAN_ACCEL_Y, SENSOR_CHAN_ACCEL_Z */
static struct sensor_stub_gen_ch_data gen_data[SENSOR_STUB_CH_CNT];

static const struct sensor_stub_gen_ch_params gen_params[SENSOR_STUB_CH_CNT] = {
	[SENSOR_STUB_CH_X] = {
		.step = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_X_STEP),
		.min  = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_X_MIN),
		.max  = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_X_MAX)
	},
	[SENSOR_STUB_CH_Y] = {
		.step = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_Y_STEP),
		.min  = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_Y_MIN),
		.max  = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_Y_MAX)
	},
	[SENSOR_STUB_CH_Z] = {
		.step = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_Z_STEP),
		.min  = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_Z_MIN),
		.max  = SENSOR_STUB_SENSOR_FP(SENSOR_STUB_GEN_Z_MAX)
	}
};

static inline int64_t sensor_val_2_fp(const struct sensor_value *val)
{
	return ((int64_t)val->val1 * SENSOR_FRACTIONAL_MAX) + val->val2;
}

static inline void fp_2_sensor_val(struct sensor_value *out, int64_t val)
{
	out->val1 = (int32_t)(val / SENSOR_FRACTIONAL_MAX);
	out->val2 = (int32_t)(val % SENSOR_FRACTIONAL_MAX);
}

/* Generate changes on selected channel with given parameters */
static inline void sensor_stub_gen_chan(struct sensor_stub_gen_ch_data *ch_data,
					const struct sensor_stub_gen_ch_params *params)
{
	int64_t val = sensor_val_2_fp(&ch_data->val);

	if (ch_data->dir) {
		val += params->step;
		if (val >= params->max) {
			ch_data->dir = false;
		}
	} else {
		val -= params->step;
		if (val <= params->min) {
			ch_data->dir = true;
		}
	}

	fp_2_sensor_val(&ch_data->val, val);
}

int sensor_stub_gen_fetch(const struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ALL:
		for (int i = 0; i < SENSOR_STUB_CH_CNT; ++i) {
			sensor_stub_gen_chan(&gen_data[i], &gen_params[i]);
		}
		LOG_DBG("Generated xyz channels: %d.%.6d, %d.%.6d, %d.%.6d",
			gen_data[SENSOR_STUB_CH_X].val.val1,
			gen_data[SENSOR_STUB_CH_X].val.val2,
			gen_data[SENSOR_STUB_CH_Y].val.val1,
			gen_data[SENSOR_STUB_CH_Y].val.val2,
			gen_data[SENSOR_STUB_CH_Z].val.val1,
			gen_data[SENSOR_STUB_CH_Z].val.val2);
		return 0;
	case SENSOR_CHAN_ACCEL_X:
		sensor_stub_gen_chan(&gen_data[SENSOR_STUB_CH_X], &gen_params[SENSOR_STUB_CH_X]);
		LOG_DBG("Generated x channel: %d.%.6d",
			gen_data[SENSOR_STUB_CH_X].val.val1,
			gen_data[SENSOR_STUB_CH_X].val.val2);
		return 0;
	case SENSOR_CHAN_ACCEL_Y:
		sensor_stub_gen_chan(&gen_data[SENSOR_STUB_CH_Y], &gen_params[SENSOR_STUB_CH_Y]);
		LOG_DBG("Generated y channel: %d.%.6d",
			gen_data[SENSOR_STUB_CH_Y].val.val1,
			gen_data[SENSOR_STUB_CH_Y].val.val2);
		return 0;
	case SENSOR_CHAN_ACCEL_Z:
		sensor_stub_gen_chan(&gen_data[SENSOR_STUB_CH_Z], &gen_params[SENSOR_STUB_CH_Z]);
		LOG_DBG("Generated z channel: %d.%.6d",
			gen_data[SENSOR_STUB_CH_Z].val.val1,
			gen_data[SENSOR_STUB_CH_Z].val.val2);
		return 0;
	default:
		return -ENOTSUP;
	}
}

int sensor_stub_gen_get(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *sample)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		for (int i = 0; i < SENSOR_STUB_CH_CNT; ++i) {
			*sample++ = gen_data[i].val;
		}
		return 0;
	case SENSOR_CHAN_ACCEL_X:
		*sample = gen_data[SENSOR_STUB_CH_X].val;
		return 0;
	case SENSOR_CHAN_ACCEL_Y:
		*sample = gen_data[SENSOR_STUB_CH_Y].val;
		return 0;
	case SENSOR_CHAN_ACCEL_Z:
		*sample = gen_data[SENSOR_STUB_CH_Z].val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

int sensor_stub_gen_init(const struct device *dev)
{
	LOG_INF("READY");
	return 0;
}
