/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ACK_EVENT_H_
#define _ACK_EVENT_H_

/**
 * @brief ACK Event
 * @defgroup ack_event ACK Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ack_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(ack_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ACK_EVENT_H_ */
