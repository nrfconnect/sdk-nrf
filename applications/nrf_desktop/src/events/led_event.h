/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_EVENT_H_
#define _LED_EVENT_H_

/**
 * @brief LED Event
 * @defgroup led_event LED Event
 * @{
 */

#include "event_manager.h"
#include "led_effect.h"


#ifdef __cplusplus
extern "C" {
#endif


/** @brief LED event used to change LED operating mode and color. */
struct led_event {
	struct event_header header;

	size_t led_id;
	const struct led_effect *led_effect;
};

EVENT_TYPE_DECLARE(led_event);

/** @brief LED ready event used to notify that leds are ready for next
 * LED stream effect.
 */
struct led_ready_event {
	struct event_header header;

	size_t led_id;
	const struct led_effect *led_effect;
};

EVENT_TYPE_DECLARE(led_ready_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_EVENT_H_ */
