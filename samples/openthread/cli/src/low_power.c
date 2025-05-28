/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <openthread.h>
#include <openthread/thread.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
#include <ram_pwrdn.h>
#endif

#include "low_power.h"

static void on_thread_state_changed(otChangedFlags flags, void *user_data)
{
	struct otInstance *instance = openthread_get_default_instance();

	if (flags & OT_CHANGED_THREAD_ROLE) {
		if (otThreadGetDeviceRole(instance) == OT_DEVICE_ROLE_CHILD) {
			const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

			if (!device_is_ready(cons)) {
				return;
			}

			pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
			power_down_unused_ram();
#endif
		}
	}
}

static struct openthread_state_changed_callback ot_state_chaged_cb = {
	.otCallback = on_thread_state_changed};

void low_power_enable(void)
{
	openthread_state_changed_callback_register(&ot_state_chaged_cb);
}
