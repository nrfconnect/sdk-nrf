/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/pm/policy.h>

#include "usb_event.h"

#define MODULE usb_state_pm
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_USB_STATE_LOG_LEVEL);

#include <caf/events/power_manager_event.h>
#include <caf/events/force_power_down_event.h>


static bool initialized;
static struct k_work_delayable pm_restrict_remove_work;

static void pm_restrict_remove_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
}

static void update_pm_policy_latency_req(bool enable)
{
	static bool enabled;
	static struct pm_policy_latency_request req;

	if (enabled == enable) {
		/* Nothing to do. */
		return;
	}

	if (enable) {
		/* Ensure no latency. */
		pm_policy_latency_request_add(&req, 0U);
	} else {
		pm_policy_latency_request_remove(&req);
	}

	enabled = enable;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_usb_state_event(aeh)) {
		const struct usb_state_event *event = cast_usb_state_event(aeh);

		LOG_DBG("USB state change detected");
		__ASSERT_NO_MSG(initialized);

		if (IS_ENABLED(CONFIG_DESKTOP_USB_PM_REQ_NO_PM_LATENCY)) {
			update_pm_policy_latency_req(event->state == USB_STATE_ACTIVE);
		}

		if ((CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS > 0) &&
		    (event->state != USB_STATE_DISCONNECTED)) {
			/* Cancel the work if it is scheduled. */
			(void)k_work_cancel_delayable(&pm_restrict_remove_work);
		}

		switch (event->state) {
		case USB_STATE_POWERED:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
			break;
		case USB_STATE_ACTIVE:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_ALIVE);
			break;
		case USB_STATE_DISCONNECTED:
			if (CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS > 0) {
				/* Remove the restriction after the delay to allow the user to
				 * take actions like e.g. restart BLE advertising without going
				 * through system off.
				 */
				(void)k_work_schedule(
					&pm_restrict_remove_work,
					K_MSEC(CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS));
			} else {
				power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
			}
			/* When disconnecting USB cable, the USB_STATE_SUSPENDED event may
			 * or may not appear before the USB_STATE_DISCONNECTED event. Add power
			 * manager restriction and force power down for application to behave
			 * consistently even if the USB_STATE_SUSPENDED event doesn't appear. Don't
			 * add restriction if the restriction removal delay is set to 0.
			 */
			if (CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS > 0) {
				power_manager_restrict(MODULE_IDX(MODULE),
						       POWER_MANAGER_LEVEL_SUSPENDED);
			}
			force_power_down();
			break;
		case USB_STATE_SUSPENDED:
			LOG_DBG("USB suspended");
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
			force_power_down();
			break;
		default:
			/* Ignore */
			break;
		}
		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(!initialized);
			if (CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS > 0) {
				k_work_init_delayable(&pm_restrict_remove_work,
						      pm_restrict_remove_work_fn);
			}
			initialized = true;
		}
		return false;
	}

	__ASSERT_NO_MSG(false);
	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
