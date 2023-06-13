/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UI_INPUT_EVENT_H__
#define UI_INPUT_EVENT_H__

#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ui_input_type {
	PUSH_BUTTON,
	ON_OFF_SWITCH
};

struct ui_input_event {
	struct app_event_header header;

	enum ui_input_type type;
	uint8_t device_number;
	bool state;
};

APP_EVENT_TYPE_DECLARE(ui_input_event);

#ifdef __cplusplus
}
#endif

#endif /* UI_INPUT_EVENT_H__ */
