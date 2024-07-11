/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <app_event_manager.h>
#include <event_manager_proxy.h>

#define MODULE main
#include <caf/events/module_state_event.h>
#include <caf/events/led_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

int main(void)
{
	const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc1));
	int err = app_event_manager_init();

	if (err) {
		LOG_ERR("Event Manager not initialized, err: %d", err);
		return err;
	}
        LOG_INF("Event manager initialized");

	err = event_manager_proxy_add_remote(ipc_instance);
        if (err && (err != -EALREADY)) {
		LOG_ERR("Cannot add remote: %d", err);
		return err;
	}
	LOG_INF("Event proxy remote added");

	err = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc_instance, led_event);
	if (err) {
		LOG_ERR("Cannot subscribe for led_event");
		return err;
	}

	err = event_manager_proxy_start();
        if (err) {
		LOG_ERR("Cannot start event manager proxy: %d", err);
		return err;
	}

	/* Wait for all the remote cores to finish event proxy initialization */
	err = event_manager_proxy_wait_for_remotes(K_MSEC(5000));
	if (err) {
		LOG_ERR("Error when waiting for remote: %d", err);
		return err;
	}

	LOG_INF("Event Manager proxy initialization finished successfully");
	module_set_state(MODULE_STATE_READY);

	return 0;
}
