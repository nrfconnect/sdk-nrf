/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PASSKEY_EVENT_H_
#define _PASSKEY_EVENT_H_

/**
 * @brief Passkey Event
 * @defgroup passkey_event Passkey Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Passkey input event. */
struct passkey_input_event {
	struct app_event_header header;

	uint32_t passkey;
};

APP_EVENT_TYPE_DECLARE(passkey_input_event);

/** @brief Passkey request event. */
struct passkey_req_event {
	struct app_event_header header;

	bool active;
};

APP_EVENT_TYPE_DECLARE(passkey_req_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PASSKEY_EVENT_H_ */
