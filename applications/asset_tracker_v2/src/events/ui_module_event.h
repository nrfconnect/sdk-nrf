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

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UI event types submitted by UI module. */
enum ui_module_event_type {
	UI_EVT_BUTTON_DATA_READY,
	UI_EVT_SHUTDOWN_READY,
	UI_EVT_ERROR
};

struct ui_module_data {
	int button_number;
	int64_t timestamp;
};

/** @brief UI event. */
struct ui_module_event {
	struct event_header header;
	enum ui_module_event_type type;

	union {
		struct ui_module_data ui;
		int err;
	} data;
};

EVENT_TYPE_DECLARE(ui_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _UI_MODULE_EVENT_H_ */
