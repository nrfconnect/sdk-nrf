/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _DATA_EVENT_H_
#define _DATA_EVENT_H_

/**
 * @brief Data Event
 * @defgroup data_event Data Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct data_event {
	struct event_header header;

	s8_t val1;
	s16_t val2;
	s32_t val3;
	u8_t val1u;
	u16_t val2u;
	u32_t val3u;

	char *descr;
};

EVENT_TYPE_DECLARE(data_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DATA_EVENT_H_ */
