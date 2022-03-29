/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_evt_mgr.h>
#include <config_event.h>
#include <logging/log.h>

#define MODULE main

LOG_MODULE_REGISTER(MODULE);

void main(void)
{
	if (app_evt_mgr_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		struct config_event *event = new_config_event();

		APPLICATION_EVENT_SUBMIT(event);
	}
}
