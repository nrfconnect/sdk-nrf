/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ORDER_EVENT_H_
#define _ORDER_EVENT_H_

/**
 * @brief Order Event
 * @defgroup order_event Order Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct order_event {
	struct event_header header;

	int val;
};

EVENT_TYPE_DECLARE(order_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ORDER_EVENT_H_ */
