/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>
#include "led_module.h"
#include "events/led_state_event.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effects associated with Asset Tracker v2 states.
 */

/* This structure enforces the header file to be included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;

enum led_id {
	LED_1,
	LED_2,
	LED_3,
	LED_4,

	LED_ID_COUNT
};

#define LED_ID_CONNECTING	LED_1
#define LED_ID_SEARCHING	LED_2
#define LED_ID_PUBLISHING	LED_3
#define LED_ID_MODE		LED_4

/* Map function to LED ID */
static const uint8_t led_map[] = {
	[LED_1] = 0,
	[LED_2] = 1,
	[LED_3] = 2,
	[LED_4] = 3
};

static const struct led_effect asset_tracker_led_effect[] = {
	[LED_STATES_LTE_CONNECTING]	= LED_EFFECT_LED_BLINK(LED_PERIOD_NORMAL,
								LED_COLOR(100, 100, 100)),
	[LED_STATES_GPS_SEARCHING]	= LED_EFFECT_LED_BLINK(LED_PERIOD_NORMAL,
								LED_COLOR(100, 100, 100)),
	[LED_STATES_CLOUD_PUBLISHING]	= LED_EFFECT_LED_BLINK(LED_PERIOD_NORMAL,
								LED_COLOR(100, 100, 100)),
	[LED_STATES_ACTIVE_MODE]	= LED_EFFECT_LED_BLINK(LED_PERIOD_NORMAL,
								LED_COLOR(100, 100, 100)),
	[LED_STATES_PASSIVE_MODE]	= LED_EFFECT_LED_BLINK(LED_PERIOD_LONG,
								LED_COLOR(100, 100, 100)),
	[LED_STATES_ERROR_SYSTEM_FAULT]	= LED_EFFECT_LED_BLINK(LED_PERIOD_NORMAL,
								LED_COLOR(100, 100, 100)),
	[LED_STATES_FOTA_UPDATE_REBOOT]	= LED_EFFECT_LED_ON(LED_COLOR(100, 100, 100)),
	[LED_STATES_TURN_OFF]		= LED_EFFECT_LED_OFF(),
};
