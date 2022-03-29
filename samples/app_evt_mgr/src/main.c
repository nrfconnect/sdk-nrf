/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_evt_mgr.h>
#include <config_event.h>
#include <logging/log.h>

#define MODULE main

LOG_MODULE_REGISTER(MODULE);

#define INIT_VALUE1 3

void main(void)
{
	if (app_evt_mgr_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		struct config_event *event = new_config_event();

		event->init_value1 = INIT_VALUE1;
		APPLICATION_EVENT_SUBMIT(event);
	}
}
