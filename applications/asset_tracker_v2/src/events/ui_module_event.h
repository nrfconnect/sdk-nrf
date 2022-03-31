/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UI_MODULE_EVENT_H_
#define _UI_MODULE_EVENT_H_

/**
 * @brief UI module event
 * @defgroup ui_module_event UI module event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UI event types submitted by the UI module. */
enum ui_module_event_type {
	/** Button has been pressed.
	 *  Payload is of type @ref ui_module_data (ui).
	 */
	UI_EVT_BUTTON_DATA_READY,

	/** The UI module has performed all procedures to prepare for
	 *  a shutdown of the system. The event carries the ID (id) of the module.
	 */
	UI_EVT_SHUTDOWN_READY,

	/** An irrecoverable error has occurred in the cloud module. Error details are
	 *  attached in the event structure.
	 */
	UI_EVT_ERROR
};

/** @brief Structure used to provide button data. */
struct ui_module_data {
	/** Button number of the board that was pressed. */
	int button_number;
	/** Uptime when the button was pressed. */
	int64_t timestamp;
};

/** @brief UI module event. */
struct ui_module_event {
	/** UI module application event header. */
	struct app_event_header header;
	/** UI module event type. */
	enum ui_module_event_type type;

	union {
		/** Variable that carries button press information. */
		struct ui_module_data ui;
		/** Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		/** Code signifying the cause of error. */
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(ui_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _UI_MODULE_EVENT_H_ */
