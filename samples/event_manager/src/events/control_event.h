/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONTROL_EVENT_H_
#define _CONTROL_EVENT_H_

/**
 * @brief Control Event
 * @defgroup control_event Control Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct control_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(control_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CONTROL_EVENT_H_ */
