/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *
 * @brief   LED module.
 *
 * Module that handles LED behaviour.
 */

#ifndef LED_H__
#define LED_H__

#include <zephyr.h>
#include <caf/led_effect.h>


#ifdef __cplusplus
extern "C" {
#endif

#define LED_PERIOD_NORMAL	500
#define LED_PERIOD_LONG		3500
#define LED_PERIOD_RAPID	200


struct asset_tracker_led_effect {
	const char *label;
	struct led_effect effect;
};


#ifdef __cplusplus
}
#endif

#endif /* LED_H__ */
