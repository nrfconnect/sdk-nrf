/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MULTICONTEXT_EVENT_H_
#define _MULTICONTEXT_EVENT_H_

/**
 * @brief Multicontext Event
 * @defgroup multicontext_event Multicontext Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct multicontext_event {
	struct event_header header;

	int val1;
	int val2;
};

EVENT_TYPE_DECLARE(multicontext_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MULTICONTEXT_H_ */
