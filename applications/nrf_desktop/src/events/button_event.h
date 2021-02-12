/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BUTTON_EVENT_H_
#define _BUTTON_EVENT_H_

/**
 * @brief Button Event
 * @defgroup button_event Button Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct button_event {
	struct event_header header;

	uint16_t key_id;
	bool  pressed;
};

EVENT_TYPE_DECLARE(button_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BUTTON_EVENT_H_ */
