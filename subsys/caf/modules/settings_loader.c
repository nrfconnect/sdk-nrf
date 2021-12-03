/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <settings/settings.h>

#include <event_manager.h>

#define MODULE settings_loader
#include <caf/events/module_state_event.h>

#include CONFIG_CAF_SETTINGS_LOADER_DEF_PATH

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_SETTINGS_LOADER_LOG_LEVEL);

#define THREAD_STACK_SIZE	CONFIG_CAF_SETTINGS_LOADER_THREAD_STACK_SIZE
#define THREAD_PRIORITY		K_PRIO_PREEMPT(K_LOWEST_APPLICATION_THREAD_PRIO)
static struct k_thread thread;
static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);


static void load_settings_thread(void)
{
	LOG_INF("Settings load thread started");

	int err = settings_load();

	if (err) {
		LOG_ERR("Cannot load settings");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		LOG_INF("Settings loaded");
		module_set_state(MODULE_STATE_READY);
	}
}

static void start_loading_thread(void)
{
	k_thread_create(&thread, thread_stack,
			THREAD_STACK_SIZE,
			(k_thread_entry_t)load_settings_thread,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread, MODULE_NAME "_thread");
}

static void load_settings(void)
{
	if (IS_ENABLED(CONFIG_CAF_SETTINGS_LOADER_USE_THREAD)) {
		start_loading_thread();
	} else {
		int err = settings_load();

		if (err) {
			LOG_ERR("Cannot load settings");
			module_set_state(MODULE_STATE_ERROR);
		} else {
			LOG_INF("Settings loaded");
			module_set_state(MODULE_STATE_READY);
		}
	}
}

static bool module_event_handler(const struct module_state_event *event)
{
	static struct module_flags req_modules_bm;

	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		static bool initialized;

		__ASSERT_NO_MSG(!initialized);
		initialized = true;

		get_req_modules(&req_modules_bm);

		/* In case user implemented empty get_req_modules function */
		if (module_flags_check_zero(&req_modules_bm)) {
			load_settings();
			return false;
		}
	}

	if (module_flags_check_zero(&req_modules_bm)) {
		/* Settings already loaded */
		return false;
	}

	if (event->state == MODULE_STATE_READY) {
		module_flags_clear_bit(&req_modules_bm, module_idx_get(event->module_id));

		if (module_flags_check_zero(&req_modules_bm)) {
			load_settings();
		}
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		return module_event_handler(cast_module_state_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
