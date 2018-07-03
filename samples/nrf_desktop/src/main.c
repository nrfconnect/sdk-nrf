/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <event_manager.h>

#include "module_state_event.h"

#define MODULE		main
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#include <logging/sys_log.h>

#if !defined(CONFIG_BOARD_NRF52_PCA20041)         && \
    !defined(CONFIG_BOARD_NRF52_DESKTOP_KEYBOARD) && \
    !defined(CONFIG_BOARD_NRF52_DESKTOP_DONGLE)
#error "Please select nRF52 Desktop board"
#endif

void main(void)
{
	if (event_manager_init()) {
		SYS_LOG_ERR("Event manager not initialized");
		return;
	}

	module_set_state("ready");
}
