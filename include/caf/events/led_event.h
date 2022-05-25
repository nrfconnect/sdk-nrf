/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_EVENT_H_
#define _LED_EVENT_H_

/**
 * @file
 * @defgroup caf_led_event CAF LED Event
 * @{
 * @brief CAF LED Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <caf/led_effect.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief LED event.
 *
 * The LED event is submitted to change effect displayed by the selected LED. The LED effect affects
 * operating mode and color of the LED. The LED effect displayed by the given LED is overridden
 * if LED receives a new LED event. For more information about LED effects see  @ref led_effect_CAF.
 */
struct led_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the LED. */
	size_t led_id;

	/** Pointer to LED effect to be displayed. */
	const struct led_effect *led_effect;
};


/** @brief LED ready event.
 *
 * The LED ready event is submitted to inform that a LED has finished displaying a LED effect.
 * The event is used to notify that the LED is ready to display next effect.
 *
 * If the displayed LED effect has loop_forever set to true, the effect never ends and the LED ready
 * event should not be submitted.
 */
struct led_ready_event {
	/** Event header. */
	struct app_event_header header;

	/** ID of the LED. */
	size_t led_id;

	/** Pointer to the LED effect that was displayed. */
	const struct led_effect *led_effect;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

APP_EVENT_TYPE_DECLARE(led_event);
APP_EVENT_TYPE_DECLARE(led_ready_event);

#ifdef __cplusplus
}
#endif

#endif /* _LED_EVENT_H_ */
