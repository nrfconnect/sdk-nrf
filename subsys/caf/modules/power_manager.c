/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <zephyr/device.h>
#include <hal/nrf_power.h>
#include <helpers/nrfx_reset_reason.h>
#include <zephyr/sys/reboot.h>

#include <app_event_manager.h>
#include <nrf_profiler.h>

#include <caf/events/power_event.h>

#define MODULE power_manager
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_POWER_MANAGER_LOG_LEVEL);

#include <caf/events/power_manager_event.h>
#include <caf/events/keep_alive_event.h>
#include <caf/events/force_power_down_event.h>


#define POWER_DOWN_ERROR_TIMEOUT      K_SECONDS(CONFIG_CAF_POWER_MANAGER_ERROR_TIMEOUT)
#define POWER_DOWN_CHECK_INTERVAL_SEC 1
#define POWER_DOWN_CHECK_INTERVAL     K_SECONDS(POWER_DOWN_CHECK_INTERVAL_SEC)


enum power_state {
	POWER_STATE_IDLE,
	POWER_STATE_SUSPENDING,
	POWER_STATE_SUSPENDED,
	POWER_STATE_OFF,
	POWER_STATE_ERROR,
	POWER_STATE_ERROR_SUSPENDED,
	POWER_STATE_ERROR_OFF
};

static enum power_state power_state;
static struct k_work_delayable  power_down_trigger;
static struct k_work_delayable  error_trigger;
static bool keep_alive_flag;
static size_t power_down_interval_counter;

/* The first state that is excluded would have a bit set.
 * It means that allowed state is one higher (lower number)
 */
static struct module_flags power_mode_restrict_flags[POWER_MANAGER_LEVEL_MAX];


static bool check_if_power_state_allowed(enum power_manager_level lvl);


static void power_down_counter_reset(void)
{
	BUILD_ASSERT(POWER_DOWN_CHECK_INTERVAL_SEC <= CONFIG_CAF_POWER_MANAGER_TIMEOUT);
	if ((power_state == POWER_STATE_IDLE) &&
	    check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED)) {
		power_down_interval_counter = 0;
		k_work_reschedule(&power_down_trigger, POWER_DOWN_CHECK_INTERVAL);
		LOG_DBG("Power down timer restarted");
	}
}

static void power_down_counter_abort(void)
{
	k_work_cancel_delayable(&power_down_trigger);
	LOG_DBG("Power down timer aborted");
}

static void set_power_state(enum power_state state)
{
	if ((power_state == POWER_STATE_IDLE) && (state != POWER_STATE_IDLE)) {
		power_down_counter_abort();
	} else if ((power_state != POWER_STATE_IDLE) && (state == POWER_STATE_IDLE)) {
		power_down_counter_reset();
	} else {
		/* Nothing to do */
	}
	power_state = state;
}

static bool check_if_power_state_allowed(enum power_manager_level lvl)
{
	enum power_manager_level current;

	if (lvl >= POWER_MANAGER_LEVEL_MAX) {
		lvl = POWER_MANAGER_LEVEL_MAX - 1;
	}

	for (current = POWER_MANAGER_LEVEL_ALIVE; current < lvl; ++current) {
		if (!module_flags_check_zero(&power_mode_restrict_flags[current + 1])) {
			return false;
		}
	}
	return true;
}

static void restrict_power_state(size_t module, enum power_manager_level lvl)
{
	enum power_manager_level current;

	for (current = POWER_MANAGER_LEVEL_ALIVE + 1;
	     current < POWER_MANAGER_LEVEL_MAX;
	     ++current) {
		if (lvl < current) {
			module_flags_set_bit(&power_mode_restrict_flags[current], module);
			break;
		}
		module_flags_clear_bit(&power_mode_restrict_flags[current], module);
	}
	LOG_DBG("Power state SUSPENDED: %s",
		check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED) ?
			"ALLOWED" : "BLOCKED");
	LOG_DBG("Power state OFF: %s",
		check_if_power_state_allowed(POWER_MANAGER_LEVEL_OFF) ?
			"ALLOWED" : "BLOCKED");
}

static void system_off(void)
{
	if (!IS_ENABLED(CONFIG_CAF_POWER_MANAGER_STAY_ON) &&
	    check_if_power_state_allowed(POWER_MANAGER_LEVEL_OFF)) {
		nrf_profiler_term();
		set_power_state(POWER_STATE_OFF);
		LOG_WRN("System turned off");
		LOG_PANIC();
		/* Clear RESETREAS to avoid starting serial recovery if nobody
		 * has cleared it already.
		 */
		nrfx_reset_reason_clear(nrfx_reset_reason_get());

		const struct pm_state_info si = {PM_STATE_SOFT_OFF, 0, 0, 0};

		if (!pm_state_force(0, &si)) {
			LOG_ERR("Failed to force soft off state");
		}
	} else {
		LOG_WRN("System suspended");
	}
}

static void system_off_on_error(void)
{
	power_state = POWER_STATE_ERROR_OFF;
	LOG_WRN("System turned off because of unrecoverable error");
	LOG_PANIC();

	const struct pm_state_info si = {PM_STATE_SOFT_OFF, 0, 0, 0};

	if (!pm_state_force(0, &si)) {
		LOG_ERR("Failed to force soft off state");
	}
}

static void power_down(struct k_work *work)
{
	if (keep_alive_flag) {
		keep_alive_flag = false;
		power_down_counter_reset();
		return;
	}

	power_down_interval_counter++;
	if (power_down_interval_counter <
	    (CONFIG_CAF_POWER_MANAGER_TIMEOUT) / (POWER_DOWN_CHECK_INTERVAL_SEC)) {
		k_work_reschedule(&power_down_trigger, POWER_DOWN_CHECK_INTERVAL);
		return;
	}

	__ASSERT_NO_MSG(power_state == POWER_STATE_IDLE);
	__ASSERT_NO_MSG(check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED));

	LOG_INF("System power down");

	struct power_down_event *event = new_power_down_event();

	event->error = false;
	set_power_state(POWER_STATE_SUSPENDING);
	APP_EVENT_SUBMIT(event);
}

static void send_wake_up(void)
{
	struct wake_up_event *event = new_wake_up_event();

	APP_EVENT_SUBMIT(event);
}

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	/* Return NULL, system should remain into PM_STATE_ACTIVE. */
	return NULL;
}

static void error(struct k_work *work)
{
	struct power_down_event *event = new_power_down_event();

	/* Turning off all the modules (including leds). */
	event->error = false;
	APP_EVENT_SUBMIT(event);
	set_power_state(POWER_STATE_ERROR_SUSPENDED);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (IS_ENABLED(CONFIG_CAF_KEEP_ALIVE_EVENTS) && is_keep_alive_event(aeh)) {
		keep_alive_flag = true;

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_FORCE_POWER_DOWN_EVENTS) && is_force_power_down_event(aeh)) {
		if (power_state != POWER_STATE_IDLE) {
			return false;
		}

		struct power_down_event *event = new_power_down_event();

		LOG_INF("Force power down processing ");
		event->error = false;
		set_power_state(POWER_STATE_SUSPENDING);
		APP_EVENT_SUBMIT(event);
		return false;
	}

	if (is_power_manager_restrict_event(aeh)) {
		const struct power_manager_restrict_event *event =
			cast_power_manager_restrict_event(aeh);

		bool was_sus_allowed = check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED);
		bool was_off_allowed = check_if_power_state_allowed(POWER_MANAGER_LEVEL_OFF);

		restrict_power_state(event->module_idx, event->level);

		bool is_sus_allowed = check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED);
		bool is_off_allowed = check_if_power_state_allowed(POWER_MANAGER_LEVEL_OFF);

		if (!was_sus_allowed && is_sus_allowed) {
			/* System suspend is now allowed - restart timer. */
			power_down_counter_reset();
		}
		if (!is_sus_allowed && was_sus_allowed) {
			/* System suspend is not allowed.
			 * Turn off suspend timer. If needed wakeup system.
			 */
			power_down_counter_abort();
			if (power_state != POWER_STATE_IDLE) {
				send_wake_up();
			}
		}

		if (!was_off_allowed && is_off_allowed) {
			/* System off is now allowed. Turn system off if needed. */
			if (power_state == POWER_STATE_SUSPENDED) {
				system_off();
			}
		}
		if (!is_off_allowed && was_off_allowed) {
			/* System off is not allowed. Reboot if needed. */
			if (power_state == POWER_STATE_OFF) {
				LOG_INF("Off restricted - rebooting");
				sys_reboot(SYS_REBOOT_WARM);
			}
		}

		return true;
	}

	if (is_power_down_event(aeh)) {
		switch (power_state) {
		case POWER_STATE_ERROR:
			k_work_reschedule(&error_trigger,
					  POWER_DOWN_ERROR_TIMEOUT);
			break;

		case POWER_STATE_ERROR_SUSPENDED:
			system_off_on_error();
			break;

		case POWER_STATE_SUSPENDING:
			LOG_INF("Power down the board");
			set_power_state(POWER_STATE_SUSPENDED);

			system_off();
			break;

		default:
			break;
		}

		return false;
	}

	if (is_wake_up_event(aeh)) {
		/* Consume event to prevent wakeup after application went into
		 * error state.
		 */
		if ((power_state == POWER_STATE_ERROR) ||
		    (power_state == POWER_STATE_ERROR_SUSPENDED)) {
			LOG_INF("Wake up event consumed");
			return true;
		}

		if ((power_state == POWER_STATE_OFF) ||
		    (power_state == POWER_STATE_ERROR_OFF)) {
			LOG_INF("Wake up when going into sleep - rebooting");
			sys_reboot(SYS_REBOOT_WARM);
		}

		LOG_INF("Wake up the board");

		set_power_state(POWER_STATE_IDLE);
		power_down_counter_reset();
		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

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
				APP_EVENT_SUBMIT(event);
				return false;
			}
		}

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			power_state = POWER_STATE_IDLE;
			initialized = true;

			LOG_INF("Activate power manager");

			k_work_init_delayable(&error_trigger, error);
			k_work_init_delayable(&power_down_trigger, power_down);
			power_down_counter_reset();
		} else if (event->state == MODULE_STATE_ERROR) {
			set_power_state(POWER_STATE_ERROR);

			struct power_down_event *event = new_power_down_event();

			event->error = true;
			APP_EVENT_SUBMIT(event);
		}

		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
#if IS_ENABLED(CONFIG_CAF_FORCE_POWER_DOWN_EVENTS)
	APP_EVENT_SUBSCRIBE(MODULE, force_power_down_event);
#endif
#if IS_ENABLED(CONFIG_CAF_KEEP_ALIVE_EVENTS)
	APP_EVENT_SUBSCRIBE(MODULE, keep_alive_event);
#endif
APP_EVENT_SUBSCRIBE(MODULE, power_manager_restrict_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, wake_up_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
