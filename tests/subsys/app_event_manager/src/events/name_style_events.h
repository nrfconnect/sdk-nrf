/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NAME_STYLE_EVENTS_H_
#define _NAME_STYLE_EVENTS_H_

/**
 * @brief Name style events
 * @defgroup name_style_event Name style events
 *
 * The events here are prepared to check the sections sorting.
 * We are sorting event subscribers by the event name and we rely
 * on the assumption that when everything is sorted alphabetically, with the right
 * postfixes added to tag subscribers begin and end, no other event would jump in between
 * other event range tags.
 * We find this not to be true when somebody does not follow our naming pattern
 * where the event always finishes with _event postfix.
 * The issue was fixed but keeping this test for the future.
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note about subscribers sorting:
 * Event manager uses _a and _z postfixes to mark subscribers start and end.
 * Now having 2 events:
 * - event_name_style
 * - event_name_style_break
 * Before fix would generate following section placement:
 *
 * - event_subscribers_event_name_style_a
 * - event_subscribers_event_name_style_break_a -> WHOOPS
 * - event_subscribers_event_name_style_break_p10_
 * - event_subscribers_event_name_style_break_z
 * - event_subscribers_event_name_style_p10_
 * - event_subscribers_event_name_style_z
 *
 * This was fixed by adding a dot before postfixes, so now the sorting order is:
 *
 * - event_subscribers_event_name_style._a
 * - event_subscribers_event_name_style._p10_
 * - event_subscribers_event_name_style._z
 * - event_subscribers_event_name_style_break._a
 * - event_subscribers_event_name_style_break._p10_
 * - event_subscribers_event_name_style_break._z
 *
 * Test stays here to make sure this would never happen again.
 */

struct event_name_style {
	struct app_event_header header;
};

struct event_name_style_break {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(event_name_style);
APP_EVENT_TYPE_DECLARE(event_name_style_break);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _NAME_STYLE_EVENTS_H_ */
