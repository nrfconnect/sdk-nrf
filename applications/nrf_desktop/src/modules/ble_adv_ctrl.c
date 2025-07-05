/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE ble_adv_ctrl
#include <caf/events/module_state_event.h>
#include <caf/events/module_suspend_event.h>
#include "usb_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_ADV_CTRL_LOG_LEVEL);

static bool ble_adv_suspended;


static void broadcast_ble_adv_req(bool suspend)
{
	if (suspend) {
		struct module_suspend_req_event *event = new_module_suspend_req_event();

		event->module_id = MODULE_ID(ble_adv);
		APP_EVENT_SUBMIT(event);
	} else {
		struct module_resume_req_event *event = new_module_resume_req_event();

		event->module_id = MODULE_ID(ble_adv);
		APP_EVENT_SUBMIT(event);
	}
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	return false;
}

static bool handle_usb_state_event(const struct usb_state_event *event)
{
	bool new_ble_adv_suspended = (event->state == USB_STATE_ACTIVE);

	if (new_ble_adv_suspended != ble_adv_suspended) {
		ble_adv_suspended = new_ble_adv_suspended;
		broadcast_ble_adv_req(ble_adv_suspended);
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB) &&
	    is_usb_state_event(aeh)) {
		return handle_usb_state_event(cast_usb_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#ifdef CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
#endif /* CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB */
