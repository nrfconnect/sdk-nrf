/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UART_DATA_EVENT_H_
#define _UART_DATA_EVENT_H_

/**
 * @brief UART Data Event
 * @defgroup uart_data_event UART Data Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Peer connection event. */
struct uart_data_event {
	struct app_event_header header;

	uint8_t dev_idx;
	uint8_t *buf;
	size_t len;
};

APP_EVENT_TYPE_DECLARE(uart_data_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _UART_DATA_EVENT_H_ */
