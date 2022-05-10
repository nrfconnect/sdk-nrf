/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/usb/usb_device.h>

#define MODULE usb_state
#include <caf/events/module_state_event.h>
#include <caf/events/power_manager_event.h>
#include <caf/events/force_power_down_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_USB_STATE_LOG_LEVEL);

#define SUSPEND_DELAY K_MSEC(1000)


/** @brief USB states. */
enum usb_state {
	USB_STATE_DISCONNECTED, /**< Cable is not attached. */
	USB_STATE_POWERED,      /**< Cable attached but no communication. */
	USB_STATE_ACTIVE,       /**< Cable attached and data is exchanged. */
	USB_STATE_SUSPENDED,    /**< Cable attached but port is suspended. */
	USB_STATE_ERROR         /**< USB error */
};

static struct k_work_delayable suspend_trigger;


static void suspend_worker(struct k_work *work)
{
	LOG_INF("Suspending");
	force_power_down();
}

static void device_status(enum usb_dc_status_code status, const uint8_t *param)
{
	static enum usb_state current_state = USB_STATE_DISCONNECTED;
	static enum usb_state before_suspend;
	enum usb_state new_state = current_state;

	switch (status) {
	case USB_DC_ERROR:
		new_state = USB_STATE_ERROR;
		module_set_state(MODULE_STATE_ERROR);
		break;
	case USB_DC_RESET:
		__ASSERT_NO_MSG(current_state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_POWERED;
		break;
	case USB_DC_CONNECTED:
		if (current_state != USB_STATE_DISCONNECTED) {
			LOG_WRN("USB_DC_CONNECTED when USB is not disconnected");
		}
		new_state = USB_STATE_POWERED;
		break;
	case USB_DC_CONFIGURED:
		__ASSERT_NO_MSG(current_state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_ACTIVE;
		break;
	case USB_DC_DISCONNECTED:
		new_state = USB_STATE_DISCONNECTED;
		break;
	case USB_DC_SUSPEND:
		__ASSERT_NO_MSG(current_state != USB_STATE_DISCONNECTED);
		before_suspend = current_state;
		new_state = USB_STATE_SUSPENDED;
		LOG_WRN("USB suspend");
		break;
	case USB_DC_RESUME:
		__ASSERT_NO_MSG(current_state != USB_STATE_DISCONNECTED);
		if (current_state == USB_STATE_SUSPENDED) {
			new_state = before_suspend;
			LOG_WRN("USB resume");
		}
		break;
	case USB_DC_INTERFACE:
	case USB_DC_SET_HALT:
	case USB_DC_CLEAR_HALT:
	case USB_DC_SOF:
		/* Ignore */
		break;
	case USB_DC_UNKNOWN:
	default:
		/* Should not happen */
		__ASSERT_NO_MSG(false);
		break;
	}

	if (current_state != new_state) {
		k_work_cancel_delayable(&suspend_trigger);
		current_state = new_state;

		switch (new_state) {
		case USB_STATE_ERROR:
		case USB_STATE_DISCONNECTED:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
			break;
		case USB_STATE_POWERED:
		case USB_STATE_ACTIVE:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_ALIVE);
			break;
		case USB_STATE_SUSPENDED:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
			k_work_schedule(&suspend_trigger, SUSPEND_DELAY);
			break;
		default:
			/* Ignore */
			break;
		}
	}
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		int err = usb_enable(device_status);

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		} else {
			k_work_init_delayable(&suspend_trigger, suspend_worker);
			module_set_state(MODULE_STATE_READY);
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
