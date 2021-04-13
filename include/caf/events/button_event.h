/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BUTTON_EVENT_H_
#define _BUTTON_EVENT_H_

/**
 * @brief CAF Button Event
 * @defgroup caf_button_event CAF Button Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct button_event {
	struct event_header header;

	uint16_t key_id;
	bool pressed;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

EVENT_TYPE_DECLARE(button_event);

#ifdef __cplusplus
}
#endif

#endif /* _BUTTON_EVENT_H_ */
