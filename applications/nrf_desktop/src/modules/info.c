/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include "hwid.h"
#include "config_event.h"

#define MODULE info
#include "module_state_event.h"

#define BOARD_NAME_SEPARATOR	'_'

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_INFO_LOG_LEVEL);

enum config_info_opt {
	INFO_OPT_BOARD_NAME,
	INFO_OPT_HWID,

	INFO_OPT_COUNT
};

const static char *opt_descr[] = {
	"board_name",
	"hwid",
};

static void fetch_config(const uint8_t opt_id, uint8_t *data, size_t *size)
{
	switch (opt_id) {
	case INFO_OPT_BOARD_NAME:
	{
		const char *start_ptr = CONFIG_BOARD;
		const char *end_ptr = strchr(start_ptr, BOARD_NAME_SEPARATOR);

		__ASSERT_NO_MSG(end_ptr != NULL);
		size_t name_len = end_ptr - start_ptr;

		__ASSERT_NO_MSG((name_len > 0) &&
			(name_len < CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE));

		strncpy(data, start_ptr, name_len);
		*size = name_len;
		break;
	}

	case INFO_OPT_HWID:
		hwid_get(data, CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE);
		*size = HWID_LEN;
		break;

	default:
		LOG_WRN("Unknown config fetch option ID %" PRIu8, opt_id);
		break;
	}
}

static void set_config(const uint8_t opt_id, const uint8_t *data, const size_t size)
{
	LOG_WRN("Config set is not supported");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr, set_config,
				  fetch_config, true);

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
