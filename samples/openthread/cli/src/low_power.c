/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <openthread/thread.h>
#include <zephyr/net/openthread.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <ram_pwrdn.h>

#include "low_power.h"

static void on_thread_state_changed(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;

	if (flags & OT_CHANGED_THREAD_ROLE) {
		if (otThreadGetDeviceRole(ot_context->instance) == OT_DEVICE_ROLE_CHILD) {
			const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

			if (!device_is_ready(cons)) {
				return;
			}

			pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
			power_down_unused_ram();
		}
	}
}

void low_power_enable(void)
{
	openthread_set_state_changed_cb(on_thread_state_changed);
}
