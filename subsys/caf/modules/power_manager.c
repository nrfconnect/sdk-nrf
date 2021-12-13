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
#include <helpers/nrfx_reset_reason.h>
#include <sys/reboot.h>

#include <event_manager.h>
#include <profiler.h>

#include <caf/events/power_event.h>

#define MODULE power_manager
#include <caf/events/module_state_event.h>

#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_POWER_MANAGER_LOG_LEVEL);

#include <caf/events/power_manager_event.h>
#include <caf/events/keep_alive_event.h>
#include <caf/events/force_power_down_event.h>

#include <settings/settings.h>
static uint32_t power_down_error_timeout		= CONFIG_CAF_POWER_MANAGER_ERROR_TIMEOUT;
static uint32_t power_down_timeout				= CONFIG_CAF_POWER_MANAGER_TIMEOUT;
#define POWER_DOWN_TIMEOUT_CONFIG_KEY		"power_down_timeout"
#define POWER_DOWN_ERROR_TIMEOUT_CONFIG_KEY	"power_down_error_timeout"

#define POWER_DOWN_CHECK_INTERVAL_SEC	1
#define POWER_DOWN_CHECK_INTERVAL		K_SECONDS(POWER_DOWN_CHECK_INTERVAL_SEC)


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
		LOG_DBG("Power down timer restarted: %d [s]", power_down_timeout);
	}
}

static void power_down_counter_abort(void)
{
	k_work_cancel_delayable(&power_down_trigger);
	LOG_DBG("Power down timer aborted");
}

static int settings_set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, POWER_DOWN_TIMEOUT_CONFIG_KEY)) {
		ssize_t len = read_cb(cb_arg, &power_down_timeout, sizeof(power_down_timeout));

		if ((len != sizeof(power_down_timeout)) || (len != len_rd)) {
			LOG_ERR(
				"Can't read power_down_timeout from storage. Using default: %d [s]",
				power_down_timeout
			);
			return len;
		}
		LOG_INF("Power down timeout set to %d [s]", power_down_timeout);
	}

	if (!strcmp(key, POWER_DOWN_ERROR_TIMEOUT_CONFIG_KEY)) {
		ssize_t len = read_cb(cb_arg, &power_down_error_timeout, sizeof(power_down_error_timeout));

		if ((len != sizeof(power_down_error_timeout)) || (len != len_rd)) {
			LOG_ERR(
				"Can't read power_down_error_timeout from storage. Using default: %d [s]",
				power_down_error_timeout
			);
			return len;
		}
		LOG_INF("Power down error timeout set to %d [s]", power_down_error_timeout);
	}

	return 0;
}

#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET)
SETTINGS_STATIC_HANDLER_DEFINE(power_manager, MODULE_NAME, NULL, settings_set, NULL, NULL);
#endif /* CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET */

static int store_power_down_error_timeout(void)
{
	if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET)) {
		char key[100]; //testing if this works
		int err = snprintk(key, sizeof(key), MODULE_NAME "/%s",
				   POWER_DOWN_ERROR_TIMEOUT_CONFIG_KEY);
		// char key[] = MODULE_NAME "/" POWER_DOWN_ERROR_TIMEOUT_CONFIG_KEY;

		err = settings_save_one(key, &power_down_error_timeout,
			sizeof(power_down_error_timeout));

		if (err) {
			LOG_ERR("Problem storing power_down_error_timeout: (err %d)", err);
			power_down_error_timeout = CONFIG_CAF_POWER_MANAGER_ERROR_TIMEOUT;
			return err;
		}
		LOG_DBG("power_down_error_timeout set to %d [s]", power_down_error_timeout);
	}
	return 0;
}

static int store_power_down_timeout(void)
{
	if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET)) {
		// char key[] = MODULE_NAME "/" POWER_DOWN_TIMEOUT_CONFIG_KEY;
		char key[30]; //testing if this works
		int err = snprintk(key, sizeof(key), MODULE_NAME "/%s",
				   POWER_DOWN_TIMEOUT_CONFIG_KEY);
		err = settings_save_one(key, &power_down_timeout,
			sizeof(power_down_timeout));

		if (err) {
			LOG_ERR("Problem storing power_down_timeout: (err %d)", err);
			power_down_timeout = CONFIG_CAF_POWER_MANAGER_TIMEOUT;
			return err;
		}
		LOG_DBG("power_down_timeout set to %d [s]", power_down_timeout);
	}
	return 0;
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
	LOG_INF("Power state SUSPENDED: %s",
		check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED) ?
			"ALLOWED" : "BLOCKED");
	LOG_INF("Power state OFF: %s",
		check_if_power_state_allowed(POWER_MANAGER_LEVEL_OFF) ?
			"ALLOWED" : "BLOCKED");
}

static void system_off(void)
{
	if (!IS_ENABLED(CONFIG_CAF_POWER_MANAGER_STAY_ON) &&
	    check_if_power_state_allowed(POWER_MANAGER_LEVEL_OFF)) {
		profiler_term();
		set_power_state(POWER_STATE_OFF);
		LOG_WRN("System turned off");
		LOG_PANIC();
		/* Clear RESETREAS to avoid starting serial recovery if nobody
		 * has cleared it already.
		 */
		nrfx_reset_reason_clear(nrfx_reset_reason_get());

		pm_power_state_force(0, (struct pm_state_info){ PM_STATE_SOFT_OFF, 0, 0 });
	} else {
		LOG_WRN("System suspended");
	}
}

static void system_off_on_error(void)
{
	power_state = POWER_STATE_ERROR_OFF;
	LOG_WRN("System turned off because of unrecoverable error");
	LOG_PANIC();

	pm_power_state_force(0, (struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
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
	    (power_down_timeout) / (POWER_DOWN_CHECK_INTERVAL_SEC)) {
		k_work_reschedule(&power_down_trigger, POWER_DOWN_CHECK_INTERVAL);
		return;
	}

	__ASSERT_NO_MSG(power_state == POWER_STATE_IDLE);
	__ASSERT_NO_MSG(check_if_power_state_allowed(POWER_MANAGER_LEVEL_SUSPENDED));

	LOG_INF("System power down");

	struct power_down_event *event = new_power_down_event();

	event->error = false;
	set_power_state(POWER_STATE_SUSPENDING);
	EVENT_SUBMIT(event);
}

static void send_wake_up(void)
{
	struct wake_up_event *event = new_wake_up_event();

	EVENT_SUBMIT(event);
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
	set_power_state(POWER_STATE_ERROR_SUSPENDED);
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_CAF_KEEP_ALIVE_EVENTS) && is_keep_alive_event(eh)) {
		keep_alive_flag = true;

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_FORCE_POWER_DOWN_EVENTS) && is_force_power_down_event(eh)) {
		if (power_state != POWER_STATE_IDLE) {
			return false;
		}

		struct power_down_event *event = new_power_down_event();

		LOG_INF("Force power down processing ");
		event->error = false;
		set_power_state(POWER_STATE_SUSPENDING);
		EVENT_SUBMIT(event);
		return false;
	}
	if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET) &&
		is_power_manager_configure_timeout_event(eh)) {
		const struct power_manager_configure_timeout_event *event =
			cast_power_manager_configure_timeout_event(eh);
		int timeout_interval;

		memcpy(&timeout_interval, &event->timeout_seconds, sizeof(timeout_interval));
		if (event->timeout_setting == POWER_DOWN_ERROR_TIMEOUT) {
			power_down_error_timeout = timeout_interval;
			// power_down_error_timeout = event->timeout_seconds;
			int err = store_power_down_error_timeout();
		} else if (event->timeout_setting == POWER_DOWN_TIMEOUT) {
			power_down_timeout = timeout_interval;
			// power_down_timeout = event->timeout_seconds;
			int err = store_power_down_timeout();
		}
		return false;
	}

	if (is_power_manager_restrict_event(eh)) {
		const struct power_manager_restrict_event *event =
			cast_power_manager_restrict_event(eh);

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
		if (is_off_allowed && !was_off_allowed) {
			/* System off is not allowed. Reboot if needed. */
			if (power_state == POWER_STATE_OFF) {
				LOG_INF("Off restricted - rebooting");
				sys_reboot(SYS_REBOOT_WARM);
			}
		}

		return true;
	}

	if (is_power_down_event(eh)) {
		switch (power_state) {
		case POWER_STATE_ERROR:
			k_work_reschedule(&error_trigger,
					  K_SECONDS(power_down_error_timeout));
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

	if (is_wake_up_event(eh)) {
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
			/* These things will be opt-out by the compiler. */
			if (!IS_ENABLED(CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET)) {
				ARG_UNUSED(settings_set);
			} else {
				settings_load();
			}

			__ASSERT_NO_MSG(!initialized);
			power_state = POWER_STATE_IDLE;

			initialized = true;

			LOG_INF("Activate power manager");

			pm_power_state_force(0, (struct pm_state_info){ PM_STATE_ACTIVE, 0, 0 });

			k_work_init_delayable(&error_trigger, error);
			k_work_init_delayable(&power_down_trigger, power_down);
			power_down_counter_reset();
		} else if (event->state == MODULE_STATE_ERROR) {
			set_power_state(POWER_STATE_ERROR);

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
#if IS_ENABLED(CONFIG_CAF_FORCE_POWER_DOWN_EVENTS)
	EVENT_SUBSCRIBE(MODULE, force_power_down_event);
#endif
#if IS_ENABLED(CONFIG_CAF_KEEP_ALIVE_EVENTS)
	EVENT_SUBSCRIBE(MODULE, keep_alive_event);
#endif
#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER_RUNTIME_TIMEOUT_SET)
	EVENT_SUBSCRIBE(MODULE, power_manager_configure_timeout_event);
#endif
EVENT_SUBSCRIBE(MODULE, power_manager_restrict_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, wake_up_event);
EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
