/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLICK_EVENT_H_
#define _CLICK_EVENT_H_

/**
 * @brief Click Event
 * @defgroup click_event Click Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Click types list. */
#define CLICK_LIST		\
	X(NONE)			\
	X(SHORT)		\
	X(LONG)			\
	X(DOUBLE)		\

/** @brief Click types. */
enum click {
#define X(name) _CONCAT(CLICK_, name),
	CLICK_LIST
#undef X
	CLICK_COUNT
};

struct click_event {
	struct event_header header;

	uint16_t key_id;
	enum click click;
};

EVENT_TYPE_DECLARE(click_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CLICK_EVENT_H_ */
