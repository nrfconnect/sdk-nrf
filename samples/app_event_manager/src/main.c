/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>
#include <config_event.h>
#include <zephyr/logging/log.h>

#define MODULE main

LOG_MODULE_REGISTER(MODULE);

#define INIT_VALUE1 3

int main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		struct config_event *event = new_config_event();

		event->init_value1 = INIT_VALUE1;
		APP_EVENT_SUBMIT(event);
	}
	return 0;
}
