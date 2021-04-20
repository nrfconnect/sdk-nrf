/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_SAMPLER_H_
#define _SENSOR_SAMPLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

enum act_type {
	ACT_TYPE_PERC,
	ACT_TYPE_ABS,
};

struct trigger_activation {
	enum act_type type;
	float thresh;
	unsigned int timeout_ms;
};

struct trigger {
	struct sensor_trigger cfg;
	struct trigger_activation activation;
};

struct sampled_channel {
	enum sensor_channel chan;
	uint8_t data_cnt;
};

struct sensor_config {
	const char *dev_name;
	const char *event_descr;
	const struct sampled_channel *chans;
	uint8_t chan_cnt;
	unsigned int sampling_period_ms;
	struct trigger *trigger;
};

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_SAMPLER_H_ */
