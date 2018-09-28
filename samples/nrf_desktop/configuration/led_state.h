/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LED_STATE_H_
#define _LED_STATE_H_

/**
 * @brief LED State
 * @defgroup led_state LED State
 * @{
 */

#include <zephyr/types.h>
#include "led_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief State of the system. */
enum system_state {
	/** Device disconnected. */
	SYSTEM_STATE_DISCONNECTED,

	/** Device connected. */
	SYSTEM_STATE_CONNECTED,

	/** Number of states. */
	SYSTEM_STATE_COUNT
};

/** @brief LEDs configuration. */
struct led_config {
	/** LED operating mode. */
	enum led_mode mode;

	/** LED light color. */
	struct led_color color;

	/** Single led brightness change time. */
	u16_t period;

};

/** @brief LED configuration defined for given board. */
extern const struct led_config led_config[SYSTEM_STATE_COUNT][CONFIG_DESKTOP_LED_COUNT];

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_STATE_H_ */
