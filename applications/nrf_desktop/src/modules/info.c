/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "hwid.h"
#include "config_event.h"

#define MODULE info
#include <caf/events/module_state_event.h>

#define BOARD_NAME_SEPARATOR	'_'

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_INFO_LOG_LEVEL);


static struct config_event *generate_response(const struct config_event *event,
					      size_t dyndata_size)
{
	__ASSERT_NO_MSG(event->is_request);

	struct config_event *rsp = new_config_event(dyndata_size);

	rsp->recipient = event->recipient;
	rsp->event_id = event->event_id;
	rsp->transport_id = event->transport_id;
	rsp->is_request = false;

	return rsp;
}

static void submit_response(struct config_event *rsp)
{
	rsp->status = CONFIG_STATUS_SUCCESS;
	APP_EVENT_SUBMIT(rsp);
}

static bool handle_config_event(const struct config_event *event)
{
	/* Not for us. */
	if (event->recipient != CFG_CHAN_RECIPIENT_LOCAL) {
		return false;
	}

	switch (event->status) {
	case CONFIG_STATUS_GET_MAX_MOD_ID:
	{
		extern const uint8_t __start_config_channel_modules[];
		extern const uint8_t __stop_config_channel_modules[];

		size_t max_mod_id = (uint8_t *)__stop_config_channel_modules -
				    (uint8_t *)__start_config_channel_modules - 1;

		__ASSERT(max_mod_id <= BIT_MASK(MOD_FIELD_SIZE),
			 "You can have up to 16 configurable modules");

		struct config_event *rsp = generate_response(event,
							     sizeof(uint8_t));

		rsp->dyndata.data[0] = max_mod_id;
		submit_response(rsp);
		return true;
	}

	case CONFIG_STATUS_GET_BOARD_NAME:
	{
		const char *start_ptr = CONFIG_BOARD;
		const char *end_ptr = strchr(start_ptr, BOARD_NAME_SEPARATOR);

		__ASSERT_NO_MSG(end_ptr != NULL);
		size_t name_len = end_ptr - start_ptr;

		__ASSERT_NO_MSG((name_len > 0) &&
			(name_len <= CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE));

		struct config_event *rsp = generate_response(event, name_len);

		strncpy((char *)rsp->dyndata.data, start_ptr, name_len);
		submit_response(rsp);
		return true;
	}

	case CONFIG_STATUS_GET_HWID:
	{
		struct config_event *rsp = generate_response(event, HWID_LEN);

		BUILD_ASSERT(HWID_LEN <= CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE);
		hwid_get(rsp->dyndata.data, HWID_LEN);
		submit_response(rsp);
		return true;
	}

	default:
		break;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_config_event(aeh)) {
		return handle_config_event(cast_config_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
