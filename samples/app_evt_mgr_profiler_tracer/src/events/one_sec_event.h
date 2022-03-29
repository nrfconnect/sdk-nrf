/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ONE_SEC_EVENT_H_
#define _ONE_SEC_EVENT_H_

/**
 * @brief One-second event
 * @defgroup one_sec_event One-second event
 * @{
 */

#include <app_evt_mgr.h>
#include <app_evt_mgr_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct one_sec_event {
	struct application_event_header header;

	int8_t five_sec_timer;
};

APPLICATION_EVENT_TYPE_DECLARE(one_sec_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ONE_SEC_EVENT_H_ */
