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

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
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

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = on_thread_state_changed
};

void low_power_enable(void)
{
	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);
}
