/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <power.h>

#include <soc_power.h>
#include <nrf_power.h>
#include <device.h>
#include <gpio.h>
#include <hal/nrf_gpiote.h>

#include <misc/printk.h>
#include <profiler.h>

#include "power_event.h"
#include "ble_event.h"

#define MODULE power_manager
#include "module_state_event.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_DESKTOP_LOG_POWER_MANAGER_LEVEL
LOG_MODULE_REGISTER(MODULE);


#define POWER_DOWN_TIMEOUT_MS	(1000 * CONFIG_DESKTOP_POWER_MANAGER_TIMEOUT)
#define DEVICE_POLICY_MAX	30

enum power_state {
	POWER_STATE_IDLE,
	POWER_STATE_SUSPENDING,
	POWER_STATE_SUSPENDED,
	POWER_STATE_TURN_OFF,
	POWER_STATE_OFF,
	POWER_STATE_WAKEUP
};

struct device_list {
	struct device	*devices;

	size_t		count;
	char		ordered[DEVICE_POLICY_MAX];
	int		retval[DEVICE_POLICY_MAX];
};

static struct device_list device_list;

static enum power_state power_state = POWER_STATE_IDLE;
static struct k_delayed_work power_down_trigger;
static unsigned int connection_count;

static void suspend_devices(struct device_list *dl)
{
	for (size_t i = 0; i < dl->count; i++) {
		size_t idx = dl->ordered[dl->count - i - 1];

		/* If necessary  the policy manager can check if a specific
		 * device in the device list is busy as shown below :
		 * if(device_busy_check(&device_list[idx])) {do something}
		 */
		int ret = device_set_power_state(&dl->devices[idx],
				DEVICE_PM_SUSPEND_STATE);

		dl->retval[dl->count - i - 1] = ret;
	}
}

/* TODO this function was copied from an example and is of poor quality.
 * Fix it. */
static void create_device_list(struct device_list *dl)
{
	/*
	 * Following is an example of how the device list can be used
	 * to suspend devices based on custom policies.
	 *
	 * Create an ordered list of devices that will be suspended.
	 * Ordering should be done based on dependencies. Devices
	 * in the beginning of the list will be resumed first.
	 */

	int count;

	device_list_get(&dl->devices, &count);


	int dcount = 3; /* Reserve for 32KHz, 16MHz and system clock */

	for (size_t i = 0;
	     (i < count) && (dcount < ARRAY_SIZE(dl->ordered));
	     i++) {
		if (!strcmp(dl->devices[i].config->name, "clk_k32src")) {
			dl->ordered[0] = i;
		} else if (!strcmp(dl->devices[i].config->name, "clk_m16src")) {
			dl->ordered[1] = i;
		} else if (!strcmp(dl->devices[i].config->name, "sys_clock")) {
			dl->ordered[2] = i;
		} else {
			dl->ordered[dcount] = i;
			dcount++;
		}
	}
	dl->count = dcount;
}

int _sys_soc_suspend(s32_t ticks)
{
	if ((ticks != K_FOREVER) && (ticks < MSEC(100))) {
		return SYS_PM_NOT_HANDLED;
	}

	if (power_state == POWER_STATE_TURN_OFF) {
		LOG_WRN("system turned off");

		power_state = POWER_STATE_OFF;

		_sys_soc_pm_idle_exit_notification_disable();
		suspend_devices(&device_list);
		_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);

		/* System is off here - wake up leads to reboot. */
		__ASSERT_NO_MSG(false);

		return SYS_PM_DEEP_SLEEP;
	}

	return SYS_PM_NOT_HANDLED;
}

static void power_down(struct k_work *work)
{
	if (power_state == POWER_STATE_IDLE) {
		LOG_INF("system power down");

		struct power_down_event *event = new_power_down_event();
		power_state = POWER_STATE_SUSPENDING;
		EVENT_SUBMIT(event);
	}
}

static void system_off(void)
{
	/* Port events are needed to leave system off state. */
	nrf_gpiote_int_enable(GPIOTE_INTENSET_PORT_Msk);
	irq_enable(GPIOTE_IRQn);

	/* Make sure that port events are enabled before
	 * the system goes off.
	 */
	__DSB();

	/* System will go off on next idle cycle. */
	power_state = POWER_STATE_TURN_OFF;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_keep_active_event(eh)) {
		switch (power_state) {
		case POWER_STATE_IDLE:
			k_delayed_work_submit(&power_down_trigger,
					POWER_DOWN_TIMEOUT_MS);
			break;
		case POWER_STATE_SUSPENDING:
		case POWER_STATE_SUSPENDED:
		case POWER_STATE_TURN_OFF:
			break;
		case POWER_STATE_OFF:
		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		LOG_INF("power down the board");

		profiler_term();

		if (connection_count > 0) {
			/* Connection is active, keep OS alive. */
			power_state = POWER_STATE_SUSPENDED;
			LOG_WRN("system suspended");
		} else {
			/* No active connection, turn system off. */
			system_off();
		}


		return false;
	}

	if (is_wake_up_event(eh)) {
		LOG_INF("wake up the board");

		power_state = POWER_STATE_IDLE;
		k_delayed_work_submit(&power_down_trigger,
				POWER_DOWN_TIMEOUT_MS);

		return false;
	}

	if (is_ble_peer_event(eh)) {
		struct ble_peer_event *event = cast_ble_peer_event(eh);

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
			/* No action */
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		if ((connection_count == 0) &&
		    (power_state == POWER_STATE_SUSPENDED)) {
			/* Last peer disconnected during standby.
			 * Turn system off.
			 */
			system_off();
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (((event->state == MODULE_STATE_OFF) ||
		     (event->state == MODULE_STATE_STANDBY)) &&
		    (power_state == POWER_STATE_SUSPENDING)) {
			/* Some module is deactivated: re-iterate */
			struct power_down_event *event = new_power_down_event();
			EVENT_SUBMIT(event);
			return false;
		}

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			LOG_INF("activate power manager");

			create_device_list(&device_list);

			k_delayed_work_init(&power_down_trigger, power_down);
			k_delayed_work_submit(&power_down_trigger,
					POWER_DOWN_TIMEOUT_MS);
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
EVENT_SUBSCRIBE(MODULE, keep_active_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
