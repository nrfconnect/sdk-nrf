/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ui_input_event.h"

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

static void log_ui_input_event(const struct app_event_header *aeh)
{
	struct ui_input_event *event = cast_ui_input_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%s event: device number = %d, state = %d",
			ui_input_type_to_string(event->type), event->device_number, event->type);
}

APP_EVENT_TYPE_DEFINE(ui_input_event,
			      log_ui_input_event,
			      NULL,
			      APP_EVENT_FLAGS_CREATE());
