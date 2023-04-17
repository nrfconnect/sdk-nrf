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
#include <zephyr/logging/log.h>

#define MODULE main_remote

LOG_MODULE_REGISTER(MODULE);

int main(void)
{
	int ret;
	const struct device *ipc_instance  = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	printk("Event Manager Proxy remote_core started\n");

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
	ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc_instance, config_event);
	if (ret) {
		LOG_ERR("Cannot register to remote config_event: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}
	LOG_INF("Event proxy config_event registered");

	ret = event_manager_proxy_start();
	if (ret) {
		LOG_ERR("Cannot start event manager proxy: %d", ret);
		__ASSERT_NO_MSG(false);
		return 0;
	}

	/* We do not have to wait for remotes here as we do not send any event that
	 * would be fatal when missed.
	 */
	LOG_INF("Initialization finished successfully");

	return 0;
}
