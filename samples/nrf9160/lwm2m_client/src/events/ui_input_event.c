/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ui_input_event.h"

#include <stdio.h>

static const char *ui_input_type_to_string(enum ui_input_type type)
{
	switch (type) {
	case PUSH_BUTTON:
		return "Button";

	case ON_OFF_SWITCH:
		return "Switch";

	default:
		return "";
	}
}

static int log_ui_input_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	struct ui_input_event *event = cast_ui_input_event(eh);

	EVENT_MANAGER_LOG(eh, "%s event: device number = %d, state = %d",
			ui_input_type_to_string(event->type), event->device_number, event->type);
	return 0;
}

EVENT_TYPE_DEFINE(ui_input_event, false, log_ui_input_event, NULL);
