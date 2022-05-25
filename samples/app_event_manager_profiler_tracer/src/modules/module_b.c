/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "five_sec_event.h"
#include "one_sec_event.h"

#define MODULE module_b


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_one_sec_event(aeh)) {
		struct one_sec_event *ose = cast_one_sec_event(aeh);

		if (ose->five_sec_timer == 5) {
			struct five_sec_event *event = new_five_sec_event();

			APP_EVENT_SUBMIT(event);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, one_sec_event);
