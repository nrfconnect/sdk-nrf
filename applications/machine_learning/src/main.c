/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);


void main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager initialization failed");
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}
