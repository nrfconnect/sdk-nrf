/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "buzzer.h"

#include <drivers/pwm.h>

#define BUZZER_PWM_NODE DT_ALIAS(buzzer_pwm)
#define BUZZER_PWM_NAME DT_LABEL(BUZZER_PWM_NODE)
#define BUZZER_PWM_PIN DT_PROP(BUZZER_PWM_NODE, ch0_pin)
#define BUZZER_PWM_FLAGS DT_PWMS_FLAGS(BUZZER_PWM_NODE)

constexpr uint16_t kBuzzerPwmPeriodUs = 10000;
constexpr uint16_t kBuzzerPwmDutyCycleUs = 5000;

static const device *sBuzzerDevice;
static bool sBuzzerState;

int BuzzerInit()
{
	sBuzzerDevice = device_get_binding(BUZZER_PWM_NAME);
	if (!sBuzzerDevice) {
		return -ENODEV;
	}
	return 0;
}

void BuzzerSetState(bool onOff)
{
	sBuzzerState = onOff;
	pwm_pin_set_usec(sBuzzerDevice, BUZZER_PWM_PIN, kBuzzerPwmPeriodUs, sBuzzerState ? kBuzzerPwmDutyCycleUs : 0,
			 BUZZER_PWM_FLAGS);
}

void BuzzerToggleState()
{
	BuzzerSetState(!sBuzzerState);
}
