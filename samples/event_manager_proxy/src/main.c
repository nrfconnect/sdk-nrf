/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#include <app_event_manager.h>
#include <event_manager_proxy.h>
#include <config_event.h>
#include <control_event.h>
#include <measurement_event.h>
#include <zephyr/logging/log.h>

#define MODULE main

LOG_MODULE_REGISTER(MODULE);

#define INIT_VALUE1 3

int main(void)
{
	int ret;
	const struct device *ipc_instance  = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	printk("Event Manager Proxy demo started\n");
	LOG_INF("Starting");

	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Event Manager not initialized, err: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event manager initialized");

	ret = event_manager_proxy_add_remote(ipc_instance);
	if (ret && ret != -EALREADY) {
		LOG_ERR("Cannot add remote: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event proxy remote added");

	/* Listen to the events from the remote */
	ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc_instance, control_event);
	if (ret) {
		LOG_ERR("Cannot register to remote control_event: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event proxy control_event registered");
	ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc_instance, measurement_event);
	if (ret) {
		LOG_ERR("Cannot register to remote measurement_event: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event proxy measurement_event registered");

	ret = event_manager_proxy_start();
	if (ret) {
		LOG_ERR("Cannot start event manager proxy: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event manager proxy started");

	/* Wait for all the remote cores to finish event proxy initialization */
	ret = event_manager_proxy_wait_for_remotes(K_MSEC(CONFIG_APP_EVENT_MANAGER_REMOTE_WAIT_TO));
	if (ret) {
		LOG_ERR("Error when waiting for remote: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("All remotes ready");

	/* Sending config event that should be received and processed by the remote */
	struct config_event *event = new_config_event();

	event->init_value1 = INIT_VALUE1;
	APP_EVENT_SUBMIT(event);

	LOG_INF("Initialization finished successfully");

	/* Main is not needed anymore - rest would be processed in events in stats module */
	return 0;
}
