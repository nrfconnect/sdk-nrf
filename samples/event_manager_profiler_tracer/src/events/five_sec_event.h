/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FIVE_SEC_EVENT_H_
#define _FIVE_SEC_EVENT_H_

/**
 * @brief Five-second event
 * @defgroup five_sec_event Five-second event
 * @{
 */

#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct five_sec_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(five_sec_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FIVE_SEC_EVENT_H_ */
