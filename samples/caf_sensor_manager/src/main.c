/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <app_event_manager.h>

#define MODULE main

#include <caf/events/module_state_event.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MODULE);

#include "event_proxy_init.h"

int main(void)
{
	int ret;

	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Event Manager not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event manager initialized");

	ret = init_event_proxy("sensor_data_aggregator_event");
	if (ret) {
		LOG_ERR("Event Manager Proxy not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}

	module_set_state(MODULE_STATE_READY);

	return 0;
}
