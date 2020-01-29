/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <power/power.h>

#include <device.h>
#include <drivers/gpio.h>
#include <hal/nrf_power.h>

#include <profiler.h>

#include "power_event.h"
#include "ble_event.h"
#include "usb_event.h"
#include "hid_event.h"

#define MODULE power_manager
#include "module_state_event.h"

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_POWER_MANAGER_LOG_LEVEL);


#define POWER_DOWN_ERROR_TIMEOUT K_SECONDS(CONFIG_DESKTOP_POWER_MANAGER_ERROR_TIMEOUT)
#define POWER_DOWN_TIMEOUT_MS	 K_SECONDS(CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT)
#define POWER_DOWN_CHECK_MS	 1000

enum power_state {
	POWER_STATE_IDLE,
	POWER_STATE_SUSPENDING,
	POWER_STATE_SUSPENDED,
	POWER_STATE_OFF,
	POWER_STATE_WAKEUP,
	POWER_STATE_ERROR,
	POWER_STATE_ERROR_SUSPENDED,
	POWER_STATE_ERROR_OFF
};


static enum power_state power_state = POWER_STATE_IDLE;
static struct k_delayed_work power_down_trigger;
static struct k_delayed_work error_trigger;
static atomic_t power_down_count;
static unsigned int connection_count;
static bool usb_connected;


static void power_down(struct k_work *work)
{
	__ASSERT_NO_MSG(!usb_connected);
	__ASSERT_NO_MSG(power_state == POWER_STATE_IDLE);

	atomic_val_t cnt = atomic_add(&power_down_count, POWER_DOWN_CHECK_MS);

	if (cnt >= POWER_DOWN_TIMEOUT_MS) {
		LOG_INF("System power down");

		struct power_down_event *event = new_power_down_event();

		event->error = false;
		power_state = POWER_STATE_SUSPENDING;
		EVENT_SUBMIT(event);
	} else {
		k_delayed_work_submit(&power_down_trigger,
				      POWER_DOWN_CHECK_MS);
	}
}

static void power_down_counter_reset(void)
{
	switch (power_state) {
	case POWER_STATE_IDLE:
		atomic_set(&power_down_count, 0);
		break;
	case POWER_STATE_SUSPENDING:
	case POWER_STATE_SUSPENDED:
	case POWER_STATE_OFF:
	case POWER_STATE_ERROR:
		/* No action */
		break;
	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

enum power_states sys_pm_policy_next_state(s32_t ticks)
{
	return SYS_POWER_STATE_ACTIVE;
}

bool sys_pm_policy_low_power_devices(enum power_states pm_state)
{
	return sys_pm_is_sleep_state(pm_state);
}

static void system_off(void)
{
	if (power_state == POWER_STATE_ERROR_SUSPENDED) {
		power_state = POWER_STATE_ERROR_OFF;
	} else {
		power_state = POWER_STATE_OFF;
	}

	LOG_WRN("System turned off");
	LOG_PANIC();

	sys_pm_force_power_state(SYS_POWER_STATE_DEEP_SLEEP_1);
}

static void error(struct k_work *work)
{
	struct power_down_event *event = new_power_down_event();

	/* Turning off all the modules (including leds). */
	event->error = false;
	EVENT_SUBMIT(event);
	power_state = POWER_STATE_ERROR_SUSPENDED;
}

static bool event_handler(const struct event_header *eh)
{
	if ((IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE) && is_hid_mouse_event(eh)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD) && is_hid_keyboard_event(eh))) {
		/* Device is connected and sends reports to host. */
		power_down_counter_reset();

		return false;
	}

	if (is_power_down_event(eh)) {
		if (power_state == POWER_STATE_ERROR) {
			k_delayed_work_submit(&error_trigger,
					      POWER_DOWN_ERROR_TIMEOUT);
			return false;
		} else if (power_state == POWER_STATE_ERROR_SUSPENDED) {
			system_off();
			return false;
		}

		if (!usb_connected && (power_state == POWER_STATE_SUSPENDING)) {
			LOG_INF("Power down the board");

			profiler_term();

			if (IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_STAY_ON) ||
			    (connection_count > 0)) {
				/* Connection is active, keep OS alive. */
				power_state = POWER_STATE_SUSPENDED;
				LOG_WRN("System suspended");
			} else {
				/* No active connection, turn system off. */
				system_off();
			}
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		/* Consume event to prevent wakeup after application went into
		 * error state.
		 */
		if ((power_state == POWER_STATE_ERROR) ||
		    (power_state == POWER_STATE_ERROR_SUSPENDED)) {
			LOG_INF("Wake up event consumed");
			return true;
		}

		if (power_state == POWER_STATE_OFF) {
			LOG_INF("Wake up when going into sleep - rebooting");
			sys_reboot(SYS_REBOOT_WARM);
		}

		LOG_INF("Wake up the board");

		power_state = POWER_STATE_IDLE;
		if (!usb_connected) {
			power_down_counter_reset();
			k_delayed_work_submit(&power_down_trigger,
					      POWER_DOWN_CHECK_MS);
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
			__ASSERT_NO_MSG(connection_count < UINT_MAX);
			connection_count++;
			break;

		case PEER_STATE_DISCONNECTED:
			__ASSERT_NO_MSG(connection_count > 0);
			connection_count--;
			break;

		case PEER_STATE_SECURED:
		case PEER_STATE_CONN_FAILED:
			/* No action */
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		if (!usb_connected && (connection_count == 0) &&
		    (power_state == POWER_STATE_SUSPENDED) &&
		    !IS_ENABLED(CONFIG_DESKTOP_POWER_MANAGER_STAY_ON)) {
			/* Last peer disconnected during standby.
			 * Turn system off.
			 */
			system_off();
		}

		power_down_counter_reset();

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_ENABLE) && is_usb_state_event(eh)) {
		const struct usb_state_event *event = cast_usb_state_event(eh);

		if ((power_state == POWER_STATE_ERROR_SUSPENDED) ||
		    (power_state == POWER_STATE_ERROR)) {
			return false;
		}

		switch (event->state) {
		case USB_STATE_POWERED:
			usb_connected = true;
			k_delayed_work_cancel(&power_down_trigger);
			if (power_state != POWER_STATE_IDLE) {
				struct wake_up_event *wue =
					new_wake_up_event();
				EVENT_SUBMIT(wue);
			}
			break;

		case USB_STATE_DISCONNECTED:
			usb_connected = false;
			power_down_counter_reset();
			k_delayed_work_submit(&power_down_trigger,
					      POWER_DOWN_CHECK_MS);
			break;

		default:
			/* Ignore */
			break;
		}
		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if ((event->state == MODULE_STATE_OFF) ||
		     (event->state == MODULE_STATE_STANDBY)) {

			if ((power_state == POWER_STATE_SUSPENDING) ||
			    (power_state == POWER_STATE_ERROR) ||
			    (power_state == POWER_STATE_ERROR_SUSPENDED)) {
				/* Some module is deactivated: re-iterate */
				struct power_down_event *event =
					new_power_down_event();

				event->error =
					(power_state == POWER_STATE_ERROR);
				EVENT_SUBMIT(event);
				return false;
			}
		}

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			LOG_INF("Activate power manager");

			sys_pm_force_power_state(SYS_POWER_STATE_ACTIVE);

			k_delayed_work_init(&error_trigger, error);
			k_delayed_work_init(&power_down_trigger, power_down);
			k_delayed_work_submit(&power_down_trigger,
					      POWER_DOWN_CHECK_MS);
		} else if (event->state == MODULE_STATE_ERROR) {
			power_state = POWER_STATE_ERROR;
			k_delayed_work_cancel(&power_down_trigger);

			struct power_down_event *event = new_power_down_event();

			event->error = true;
			EVENT_SUBMIT(event);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, usb_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, wake_up_event);
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
