/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SIZED_EVENTS_H_
#define _SIZED_EVENTS_H_

/**
 * @brief Events with different sizes
 * @defgroup sized_events Events used to test @ref app_evt_mgr_event_size
 * @{
 */

#include <app_evt_mgr.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct test_size1_event {
	struct application_event_header header;

	uint8_t val1;
};

APPLICATION_EVENT_TYPE_DECLARE(test_size1_event);


struct test_size2_event {
	struct application_event_header header;

	uint8_t val1;
	uint8_t val2;
};

APPLICATION_EVENT_TYPE_DECLARE(test_size2_event);


struct test_size3_event {
	struct application_event_header header;

	uint8_t val1;
	uint8_t val2;
	uint8_t val3;
};

APPLICATION_EVENT_TYPE_DECLARE(test_size3_event);


struct test_size_big_event {
	struct application_event_header header;

	uint32_t array[64];
};

APPLICATION_EVENT_TYPE_DECLARE(test_size_big_event);


struct test_dynamic_event {
	struct application_event_header header;

	struct event_dyndata dyndata;
};

APPLICATION_EVENT_TYPE_DYNDATA_DECLARE(test_dynamic_event);


struct test_dynamic_with_data_event {
	struct application_event_header header;

	uint32_t val1;
	uint32_t val2;
	struct event_dyndata dyndata;
};

APPLICATION_EVENT_TYPE_DYNDATA_DECLARE(test_dynamic_with_data_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SIZED_EVENTS_H_ */
