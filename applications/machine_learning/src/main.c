/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_evt_mgr.h>

#define MODULE main
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE);


void main(void)
{
	if (app_evt_mgr_init()) {
		LOG_ERR("Application Event Manager initialization failed");
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}
