/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LED_EVENT_H_
#define _LED_EVENT_H_

/**
 * @brief LED Event
 * @defgroup led_event LED Event
 * @{
 */

#include "event_manager.h"


#ifdef __cplusplus
extern "C" {
#endif


/** @brief LED colors values. */
struct led_color {
	u8_t c[CONFIG_DESKTOP_LED_COLOR_COUNT];
};

/** @brief LED operating modes list. */
#define LED_MODE_LIST		\
	X(ON)			\
	X(OFF)			\
	X(BLINKING)

/** @brief LED operating modes. */
enum led_mode {
#define X(name) _CONCAT(LED_MODE_, name),
	LED_MODE_LIST
#undef X

	LED_MODE_COUNT
};

/** @brief LED event used to change LED operating mode and color. */
struct led_event {
	struct event_header header;

	size_t led_id;
	enum led_mode mode;
	struct led_color color;
	u16_t period;
};

EVENT_TYPE_DECLARE(led_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_EVENT_H_ */
