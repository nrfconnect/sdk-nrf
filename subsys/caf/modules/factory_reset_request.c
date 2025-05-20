/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <app_event_manager.h>

#include <caf/events/button_event.h>
#include <caf/events/factory_reset_event.h>

#define MODULE factory_reset_request
#include <caf/events/module_state_event.h>


static struct k_work_delayable init_work;


static void init_work_handler(struct k_work *work)
{
	module_set_state(MODULE_STATE_READY);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_button_event(aeh)) {
		struct button_event *event = cast_button_event(aeh);

		if (event->key_id != CONFIG_CAF_FACTORY_RESET_REQUEST_BUTTON) {
			return false;
		}

		if (k_work_delayable_is_pending(&init_work) && event->pressed) {
			APP_EVENT_SUBMIT(new_factory_reset_event());
		}
		return false;
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(buttons), MODULE_STATE_READY)) {
			k_work_init_delayable(&init_work, init_work_handler);
			k_work_schedule(&init_work,
					K_MSEC(CONFIG_CAF_FACTORY_RESET_REQUEST_DELAY));
		}
		return false;
	}
	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
