/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <event_manager.h>
#include <config_event.h>
#include <logging/log.h>

#define MODULE main

LOG_MODULE_REGISTER(MODULE);

void main(void)
{
	if (event_manager_init()) {
		LOG_ERR("Event Manager not initialized");
	} else {
		struct config_event *event = new_config_event();

		EVENT_SUBMIT(event);
	}
}
