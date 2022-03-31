/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONFIG_EVENT_H_
#define _CONFIG_EVENT_H_

/**
 * @brief Config Event
 * @defgroup config_event Config Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct config_event {
	struct app_event_header header;

	int8_t init_value1;
};

APP_EVENT_TYPE_DECLARE(config_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CONFIG_EVENT_H_ */
