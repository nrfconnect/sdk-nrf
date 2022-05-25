/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DATA_EVENTS_H_
#define _DATA_EVENTS_H_

/**
 * @brief Data Event
 * @defgroup data_event Data Event
 * @{
 */

#include <app_event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Size of the big data block in words.
 */
#define DATA_BIG_EVENT_BLOCK_SIZE 64

/**
 * @brief The event with some data to test.
 *
 * This event contains a few values with different types and sizes to test.
 * The event is intended to be sent from the host to the remote.
 *
 * @sa data_response_event
 */
struct data_event {
	struct app_event_header header;

	int8_t val1;
	int16_t val2;
	int32_t val3;
	uint8_t val1u;
	uint16_t val2u;
	uint32_t val3u;
};

APP_EVENT_TYPE_DECLARE(data_event);

/**
 * @brief The event with some data to test.
 *
 * This event contains a few values with different types and sizes to test.
 * The event is intended to be sent the remote to the host.
 *
 * @sa data_event
 */
struct data_response_event {
	struct app_event_header header;

	int8_t val1;
	int16_t val2;
	int32_t val3;
	uint8_t val1u;
	uint16_t val2u;
	uint32_t val3u;
};

APP_EVENT_TYPE_DECLARE(data_response_event);

/**
 * @brief The event with big chunk of data to test.
 *
 * This event contains big block of data to test.
 * The event is intended to be sent from the host to the remote.
 *
 * @sa  data_big_response_event
 */
struct data_big_event {
	struct app_event_header header;

	uint32_t block[DATA_BIG_EVENT_BLOCK_SIZE];
};

APP_EVENT_TYPE_DECLARE(data_big_event);

/**
 * @brief The event with big chunk of data to test.
 *
 * This event contains big block of data to test.
 * The event is intended to be sent from the remote to the host.
 *
 * @sa  data_big_event
 */
struct data_big_response_event {
	struct app_event_header header;

	uint32_t block[DATA_BIG_EVENT_BLOCK_SIZE];
};

APP_EVENT_TYPE_DECLARE(data_big_response_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DATA_EVENTS_H_ */
