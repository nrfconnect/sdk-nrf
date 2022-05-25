/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "pwm_led.h"
#include <nrfx.h>
#include <nrfx_gpiote.h>
#include <zephyr/drivers/pwm.h>

#define PWM_LED0_NODE	DT_ALIAS(pwm_led0)

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
static const struct pwm_dt_spec led0 = PWM_DT_SPEC_GET(PWM_LED0_NODE);
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#endif

#define PWM_PERIOD	1024
#define LIGHTNESS_MAX	UINT16_MAX

int pwm_led_init(void)
{
	if (!device_is_ready(led0.dev)) {
		return -1;
	}

	return 0;
}

void pwm_led_set(uint16_t desired_lvl)
{
	uint32_t scaled_lvl = (PWM_PERIOD * desired_lvl) / LIGHTNESS_MAX;

	pwm_set_dt(&led0, PWM_USEC(PWM_PERIOD), PWM_USEC(scaled_lvl));
}
