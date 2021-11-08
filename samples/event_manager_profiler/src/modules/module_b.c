/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include "five_sec_event.h"
#include "one_sec_event.h"

#define MODULE module_b


static bool event_handler(const struct event_header *eh)
{
	if (is_one_sec_event(eh)) {
		struct one_sec_event *ose = cast_one_sec_event(eh);

		if (ose->five_sec_timer == 5) {
			struct five_sec_event *event = new_five_sec_event();

			EVENT_SUBMIT(event);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, one_sec_event);
