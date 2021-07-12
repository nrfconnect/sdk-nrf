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


static bool event_handler(const struct event_header *eh)
{
	__ASSERT_NO_MSG(is_hid_report_event(eh));
	keep_alive();
	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, hid_report_event);
