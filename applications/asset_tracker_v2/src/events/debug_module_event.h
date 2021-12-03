/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DEBUG_MODULE_EVENT_H_
#define _DEBUG_MODULE_EVENT_H_

/**
 * @brief Debug module event
 * @defgroup debug_module_event Debug module event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_DEBUG_MODULE_MEMFAULT_USE_EXTERNAL_TRANSPORT)
#define MEMFAULT_BUFFER_SIZE_MAX CONFIG_DEBUG_MODULE_MEMFAULT_CHUNK_SIZE_MAX
#else
#define MEMFAULT_BUFFER_SIZE_MAX 0
#endif /* if defined(CONFIG_DEBUG_MODULE_MEMFAULT_USE_EXTERNAL_TRANSPORT) */

enum debug_module_event_type {
	/** Event carrying memfault data that should be forwarded via the configured cloud
	 *  backend to Memfault cloud. Only sent if
	 *  CONFIG_DEBUG_MODULE_MEMFAULT_USE_EXTERNAL_TRANSPORT is enabled.
	 *  Payload is of type @ref debug_module_memfault_data.
	 */
	DEBUG_EVT_MEMFAULT_DATA_READY,

	/** An irrecoverable error has occurred in the debug module. Error details are
	 *  attached in the event structure.
	 */
	DEBUG_EVT_ERROR
};

struct debug_module_memfault_data {
	uint8_t buf[MEMFAULT_BUFFER_SIZE_MAX];
	size_t len;
};

/** @brief Debug event. */
struct debug_module_event {
	struct event_header header;
	enum debug_module_event_type type;

	union {
		struct debug_module_memfault_data memfault;
		/** Code signifying the cause of error. */
		int err;
	} data;
};

EVENT_TYPE_DECLARE(debug_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DEBUG_MODULE_EVENT_H_ */
