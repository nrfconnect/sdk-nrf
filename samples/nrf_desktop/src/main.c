/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <event_manager.h>

#define MODULE main
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BUTTONS_MODULE_LEVEL
#include <logging/sys_log.h>


void main(void)
{
	if (event_manager_init()) {
		SYS_LOG_ERR("Event manager not initialized");
		return;
	}

	module_set_state(MODULE_STATE_READY);
}
