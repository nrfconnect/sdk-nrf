/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLICK_EVENT_H_
#define _CLICK_EVENT_H_

/**
 * @brief CAF Click Event
 * @defgroup caf_click_event CAF Click Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Click types. */
enum click {
	CLICK_NONE,
	CLICK_SHORT,
	CLICK_LONG,
	CLICK_DOUBLE,

	CLICK_COUNT
};

struct click_event {
	struct event_header header;

	uint16_t key_id;
	enum click click;
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

EVENT_TYPE_DECLARE(click_event);

#ifdef __cplusplus
}
#endif

#endif /* _CLICK_EVENT_H_ */
