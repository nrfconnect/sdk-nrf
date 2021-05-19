/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <zephyr/types.h>
#include <pm/pm.h>

#include <device.h>
#include <drivers/gpio.h>
#include <hal/nrf_power.h>

#include <event_manager.h>
#include <profiler.h>

#include <caf/events/power_event.h>

#define MODULE power_manager
#include <caf/events/module_state_event.h>

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_POWER_MANAGER_LOG_LEVEL);

#include "keep_alive_event.h"


#define POWER_DOWN_ERROR_TIMEOUT \
	K_SECONDS(CONFIG_PELION_CLIENT_POWER_MANAGER_ERROR_TIMEOUT)
#define POWER_DOWN_TIMEOUT_MS \
	(CONFIG_PELION_CLIENT_POWER_MANAGER_TIMEOUT * MSEC_PER_SEC)
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
static struct k_work_delayable power_down_trigger;
static struct k_work_delayable error_trigger;
static atomic_t power_down_count;


static bool system_off_check(void)
{
	return !IS_ENABLED(CONFIG_PELION_CLIENT_POWER_MANAGER_STAY_ON) &&
	       (power_state == POWER_STATE_SUSPENDED);
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

	pm_power_state_force((struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
}

static void power_down(struct k_work *work)
{
	atomic_val_t cnt = atomic_add(&power_down_count, POWER_DOWN_CHECK_MS);

	if (cnt >= POWER_DOWN_TIMEOUT_MS) {
		if (system_off_check()) {
			system_off();
		} else if (power_state != POWER_STATE_SUSPENDED) {
			LOG_INF("System power down");

			struct power_down_event *event = new_power_down_event();

			event->error = false;
			power_state = POWER_STATE_SUSPENDING;
			EVENT_SUBMIT(event);
		} else {
			/* Stay suspended. */
		}
	} else {
		k_work_reschedule(&power_down_trigger,
				      K_MSEC(POWER_DOWN_CHECK_MS));
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

struct pm_state_info pm_policy_next_state(int32_t ticks)
{
	return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
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
	if (is_keep_alive_event(eh)) {
		power_down_counter_reset();

		return false;
	}

	if (is_power_down_event(eh)) {
		switch (power_state) {
		case POWER_STATE_ERROR:
			k_work_reschedule(&error_trigger,
					      POWER_DOWN_ERROR_TIMEOUT);
			break;

		case POWER_STATE_ERROR_SUSPENDED:
			system_off();
			break;

		case POWER_STATE_SUSPENDING:
			LOG_INF("Power down the board");
			profiler_term();
			power_state = POWER_STATE_SUSPENDED;
			if (system_off_check()) {
				system_off();
			} else {
				LOG_WRN("System suspended");
			}
			break;

		default:
			/* Ignore. */
			break;
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
		power_down_counter_reset();
		k_work_reschedule(&power_down_trigger,
				      K_MSEC(POWER_DOWN_CHECK_MS));

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

			pm_power_state_force((struct pm_state_info){PM_STATE_ACTIVE, 0, 0});

			k_work_init_delayable(&error_trigger, error);
			k_work_init_delayable(&power_down_trigger, power_down);
			k_work_reschedule(&power_down_trigger,
					      K_MSEC(POWER_DOWN_CHECK_MS));
		} else if (event->state == MODULE_STATE_ERROR) {
			power_state = POWER_STATE_ERROR;
			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&power_down_trigger);

			struct power_down_event *event = new_power_down_event();

			event->error = true;
			EVENT_SUBMIT(event);
		}

		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, keep_alive_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, wake_up_event);
EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
