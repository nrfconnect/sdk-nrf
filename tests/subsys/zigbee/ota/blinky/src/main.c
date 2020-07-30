/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <dk_buttons_and_leds.h>

void main(void)
{
	int blink_status = 0;

	dk_leds_init();

	while (1) {
		blink_status ^= CONFIG_BLINKY_LED_MASK;
		dk_set_leds(blink_status);
		k_sleep(K_MSEC(100));
	}
}
