/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_STATE_H_
#define _LED_STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <caf/led_effect.h>

#define LED_UNAVAILABLE		0xFF
#define ANOMALY_LABEL		"_anomaly"

struct ml_result_led_effect {
	const char *label;
	struct led_effect effect;
};

enum led_id {
	LED_ID_ML_STATE,
	LED_ID_SENSOR_SIM,

	LED_ID_COUNT
};

#ifdef __cplusplus
}
#endif

#endif /* _LED_STATE_H_ */
