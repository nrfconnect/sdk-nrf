/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "pwm_led.h"
#include <nrfx.h>
#include <nrfx_gpiote.h>
#include <drivers/pwm.h>

#define PWM_LED0_NODE	DT_ALIAS(pwm_led0)

/*
 * Devicetree helper macro which gets the 'flags' cell from the node's
 * pwms property, or returns 0 if the property has no 'flags' cell.
 */

#if DT_PHA_HAS_CELL(PWM_LED0_NODE, pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(PWM_LED0_NODE)
#else
#define PWM_FLAGS 0
#endif

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#define PWM_CTLR	DT_PWMS_CTLR(PWM_LED0_NODE)
#define PWM_CHANNEL	DT_PWMS_CHANNEL(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_CTLR	DT_INVALID_NODE
#define PWM_CHANNEL	0
#endif

#define PWM_PERIOD	1024
#define LIGHTNESS_MAX	UINT16_MAX

static const struct device *pwm;

int pwm_led_init(void)
{
	pwm = DEVICE_DT_GET(PWM_CTLR);
	if (!device_is_ready(pwm)) {
		return -1;
	}

	return 0;
}

void pwm_led_set(uint16_t desired_lvl)
{
	uint32_t scaled_lvl = (PWM_PERIOD * desired_lvl) / LIGHTNESS_MAX;

	pwm_pin_set_usec(pwm, PWM_CHANNEL, PWM_PERIOD, scaled_lvl, PWM_FLAGS);
}
