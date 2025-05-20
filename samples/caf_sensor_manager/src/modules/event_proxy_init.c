/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE event_proxy_init

#include <zephyr/ipc/ipc_service.h>
#include <event_manager_proxy.h>
#include <caf/events/sensor_data_aggregator_event.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MODULE);

int init_event_proxy(char *proxy_event_name)
{
	const struct device *ipc_instance  = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	int ret;

	ret = event_manager_proxy_add_remote(ipc_instance);
	if (ret) {
		LOG_ERR("Cannot add remote: %d", ret);
		__ASSERT_NO_MSG(false);
		return ret;
	}
	LOG_INF("Event proxy remote added");

	if (strcmp(proxy_event_name, "sensor_data_aggregator_event") == 0) {
		ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc_instance, sensor_data_aggregator_event);
	} else if (strcmp(proxy_event_name,
			  "sensor_data_aggregator_release_buffer_event") == 0) {
		ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc_instance,
						    sensor_data_aggregator_release_buffer_event);
	}
	if (ret) {
		LOG_ERR("Cannot register to remote %s: %d", proxy_event_name, ret);
		__ASSERT_NO_MSG(false);
		return ret;
	}
	LOG_INF("Event proxy %s registered", proxy_event_name);

	ret = event_manager_proxy_start();
	if (ret) {
		LOG_ERR("Cannot start event manager proxy: %d", ret);
		__ASSERT_NO_MSG(false);
		return ret;
	}
	LOG_INF("Event manager proxy started");

	ret = event_manager_proxy_wait_for_remotes(
		K_MSEC(CONFIG_APP_SENSOR_MANAGER_REMOTE_CORE_INITIALIZATION_TIMEOUT));
	if (ret) {
		LOG_ERR("Error when waiting for remote: %d", ret);
		__ASSERT_NO_MSG(false);
		return ret;
	}
	LOG_INF("All remotes ready");

	return ret;
}
