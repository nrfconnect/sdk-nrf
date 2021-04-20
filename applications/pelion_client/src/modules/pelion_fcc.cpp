/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include "factory_configurator_client.h"
#include "pelion_fcc_err.h"

#include <caf/events/button_event.h>

#define MODULE pelion_fcc
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_PELION_LOG_LEVEL);


static struct k_delayed_work fcc_init_work;
static bool reset_storage;


static void fcc_init_work_handler(struct k_work *work)
{
	fcc_status_e err = fcc_init();
	if (err != FCC_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize FCC (err:%s)", fcc_status_to_string(err));
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	if (reset_storage) {
		LOG_WRN("Storage reset requested");
		err = fcc_storage_delete();
		if (err != FCC_STATUS_SUCCESS) {
			LOG_ERR("Failed to reset storage (err:%s)", fcc_status_to_string(err));
			module_set_state(MODULE_STATE_ERROR);
			return;
		}
	}

	err = fcc_developer_flow();
	switch (err) {
	case FCC_STATUS_KCM_FILE_EXIST_ERROR:
		LOG_INF("Factory configuration already stored");
		break;
	case FCC_STATUS_SUCCESS:
		LOG_INF("Factory configuration stored");
		break;
	default:
		LOG_ERR("Failed to initialize developer flow (err:%s)", fcc_status_to_string(err));
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	module_set_state(MODULE_STATE_READY);
}


static bool handle_button_event(const struct button_event *event)
{
	if (k_delayed_work_pending(&fcc_init_work) &&
	    event->pressed) {
		reset_storage = true;
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);
		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&fcc_init_work, fcc_init_work_handler);
			k_delayed_work_submit(&fcc_init_work, K_MSEC(50));
		}

		return false;
	}

	if (is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, button_event);
