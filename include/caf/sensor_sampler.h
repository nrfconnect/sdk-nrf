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

struct sampled_channel {
	enum sensor_channel chan;
	uint8_t data_cnt;
};

struct sensor_config {
	const char *dev_name;
	const char *event_descr;
	const struct sampled_channel *chans;
	unsigned int sampling_period_ms;
	uint8_t chan_cnt;
};

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_SAMPLER_H_ */
