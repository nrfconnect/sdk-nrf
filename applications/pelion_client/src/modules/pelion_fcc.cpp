/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include "factory_configurator_client.h"
#include "pelion_fcc_err.h"


#define MODULE pelion_fcc
#include <caf/events/module_state_event.h>
#include <caf/events/factory_reset_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_PELION_LOG_LEVEL);

#define NET_MODULE_ID_STATE_READY_WAIT_FOR                          \
	(IS_ENABLED(CONFIG_CAF_FACTORY_RESET_REQUEST) ?             \
		MODULE_ID(factory_reset_request) : MODULE_ID(main))

static bool initialized;


static bool handle_state_event(const struct module_state_event *event)
{
	if (!check_state(event, NET_MODULE_ID_STATE_READY_WAIT_FOR, MODULE_STATE_READY)) {
		return false;
	}

	fcc_status_e err;

	__ASSERT_NO_MSG(!initialized);

	err = fcc_init();
	if (err != FCC_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize FCC (err:%s)", fcc_status_to_string(err));
		module_set_state(MODULE_STATE_ERROR);
		return false;
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
		return false;
	}

	module_set_state(MODULE_STATE_READY);
	initialized = true;
	return false;
}

static bool handle_factory_reset(void)
{
	fcc_status_e err;
	/* This is expected to be called before initialization */
	__ASSERT_NO_MSG(!initialized);
	LOG_WRN("Storage reset requested");

	/* FCC needs to be initialized to clear it.
	 * This is not an issue as the fcc_init function would finish
	 * silently if already initialized.
	 */
	err = fcc_init();
	if (err != FCC_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize FCC (err:%s)", fcc_status_to_string(err));
		module_set_state(MODULE_STATE_ERROR);
		return false;
	}

	err = fcc_storage_delete();
	if (err != FCC_STATUS_SUCCESS) {
		LOG_ERR("Failed to reset storage (err:%s)", fcc_status_to_string(err));
		module_set_state(MODULE_STATE_ERROR);
		return false;
	}
	return false;
}


static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		return handle_state_event(cast_module_state_event(eh));
	}

	if (IS_ENABLED(CONFIG_CAF_FACTORY_RESET_EVENTS) &&
	    is_factory_reset_event(eh)) {
		return handle_factory_reset();
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if IS_ENABLED(CONFIG_CAF_FACTORY_RESET_EVENTS)
	EVENT_SUBSCRIBE(MODULE, factory_reset_event);
#endif
