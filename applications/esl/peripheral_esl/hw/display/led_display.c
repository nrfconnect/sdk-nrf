/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>

#include "esl.h"
LOG_MODULE_DECLARE(peripheral_esl);

int display_init(void)
{
	LOG_INF("Simulated display device init");

	return 0;
}

/* Use DK LED3 to simulate Display */
int display_control(uint8_t disp_idx, uint8_t img_idx, bool enable)
{
	ARG_UNUSED(disp_idx);
	ARG_UNUSED(enable);
	LOG_DBG("Simulated display blink LED %d img_idx times %d", SIMULATE_DISPLAY_LED_IDX,
		img_idx);
	img_idx = (img_idx + 1) + CONFIG_ESL_SIMULATE_DISPLAY_BLINK_TIMES;
	for (size_t idx = 0; idx <= img_idx; idx++) {
		dk_set_led(SIMULATE_DISPLAY_LED_IDX, 1);
		k_msleep(100);
		dk_set_led(SIMULATE_DISPLAY_LED_IDX, 0);
		k_msleep(100);
	}

	return 0;
}

void display_unassociated(uint8_t disp_idx)
{
	ARG_UNUSED(disp_idx);
	LOG_DBG("Simulate display device unassociated LED %d", SIMULATE_DISPLAY_LED_IDX);
	for (size_t idx = 0; idx <= CONFIG_ESL_SIMULATE_DISPLAY_BLINK_TIMES; idx++) {
		dk_set_led(SIMULATE_DISPLAY_LED_IDX, 1);
		k_msleep(100);
		dk_set_led(SIMULATE_DISPLAY_LED_IDX, 0);
		k_msleep(100);
	}
}

void display_associated(uint8_t disp_idx)
{
	display_unassociated(disp_idx);
}
