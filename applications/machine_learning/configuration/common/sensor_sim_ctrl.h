/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_SIM_CTRL_H_
#define _SENSOR_SIM_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>
#include <wave_gen.h>

struct sim_wave {
	const char *label;
	struct wave_gen_param wave_param;
};

struct sim_signal_params {
	enum sensor_channel chan;
	const struct sim_wave *waves;
	uint8_t waves_cnt;
};

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_SIM_CTRL_H_ */
