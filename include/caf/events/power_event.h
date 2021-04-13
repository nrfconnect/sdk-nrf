/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _POWER_EVENT_H_
#define _POWER_EVENT_H_

/**
 * @brief CAF Power Event
 * @defgroup caf_power_event CAF Power Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct power_down_event {
	struct event_header header;
	bool error;
};


struct wake_up_event {
	struct event_header header;
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

EVENT_TYPE_DECLARE(power_down_event);
EVENT_TYPE_DECLARE(wake_up_event);

#ifdef __cplusplus
}
#endif

#endif /* _POWER_EVENT_H_ */
