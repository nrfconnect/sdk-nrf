/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SELECTOR_EVENT_H_
#define _SELECTOR_EVENT_H_

/**
 * @brief Selector Event
 * @defgroup selector_event Selector Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


struct selector_event {
	struct app_event_header header;

	uint8_t selector_id;
	uint8_t position;
};

APP_EVENT_TYPE_DECLARE(selector_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SELECTOR_EVENT_H_ */
