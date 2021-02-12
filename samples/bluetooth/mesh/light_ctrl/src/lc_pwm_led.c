/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
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
#define PWM_LABEL	DT_PWMS_LABEL(PWM_LED0_NODE)
#define PWM_CHANNEL	DT_PWMS_CHANNEL(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_LABEL	""
#define PWM_CHANNEL	0
#endif

static const struct device *pwm;
#define PWM_PERIOD 1024

void lc_pwm_led_init(void)
{
	pwm = device_get_binding(PWM_LABEL);
	if (!pwm) {
		printk("Error: didn't find %s device\n", PWM_LABEL);
		return;
	}
}

void lc_pwm_led_set(uint16_t desired_lvl)
{
	uint32_t scaled_lvl =
		(PWM_PERIOD * desired_lvl) /
		BT_MESH_LIGHTNESS_MAX;

	pwm_pin_set_usec(pwm,
		PWM_CHANNEL,
		PWM_PERIOD,
		scaled_lvl,
		PWM_FLAGS);
}
