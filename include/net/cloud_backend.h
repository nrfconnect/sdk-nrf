/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZEPHYR_INCLUDE_CLOUD_BACKEND_H_
#define ZEPHYR_INCLUDE_CLOUD_BACKEND_H_

#include <zephyr.h>
#include <net/cloud.h>


/**
 * @brief Calls the user-provided event handler with event data.
 *
 * @param backend 	Pointer to cloud backend.
 * @param evt		Pointer to event data.
 * @param user_data	Pointer to user-defined data.
 */
static inline void cloud_notify_event(struct cloud_backend *backend,
			struct cloud_event *evt,
			void *user_data)
{
	if (backend->config->handler) {
		backend->config->handler(backend, evt, user_data);
	}
}

#endif /* ZEPHYR_INCLUDE_CLOUD_BACKEND_H_ */
