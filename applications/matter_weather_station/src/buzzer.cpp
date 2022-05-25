/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "buzzer.h"

#include <zephyr/drivers/pwm.h>

static const struct pwm_dt_spec sBuzzer = PWM_DT_SPEC_GET(DT_ALIAS(buzzer_pwm));

static bool sBuzzerState;

int BuzzerInit()
{
	if (!device_is_ready(sBuzzer.dev)) {
		return -ENODEV;
	}
	return 0;
}

void BuzzerSetState(bool onOff)
{
	sBuzzerState = onOff;
	pwm_set_pulse_dt(&sBuzzer, sBuzzerState ? (sBuzzer.period / 2) : 0);
}

void BuzzerToggleState()
{
	BuzzerSetState(!sBuzzerState);
}
