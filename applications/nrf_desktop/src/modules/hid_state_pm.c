/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include "hid_event.h"

#define MODULE hid_state_pm
#include <caf/events/module_state_event.h>
#include <caf/events/keep_alive_event.h>


static bool app_event_handler(const struct app_event_header *aeh)
{
	__ASSERT_NO_MSG(is_hid_report_event(aeh));
	keep_alive();
	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
