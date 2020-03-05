/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include "config_event.h"

#define MODULE info
#include "module_state_event.h"

#define BOARD_PREFIX "pca"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_INFO_LOG_LEVEL);

enum config_info_opt {
	INFO_OPT_BOARD_NAME,

	INFO_OPT_COUNT
};

const static char *opt_descr[] = {
	"board_name"
};

static void fetch_config(const u8_t opt_id, u8_t *data, size_t *size)
{
	switch (opt_id) {
	case INFO_OPT_BOARD_NAME:
	{
		const char *name_ptr = strstr(CONFIG_BOARD, BOARD_PREFIX);
		size_t name_len = strlen(name_ptr);

		__ASSERT_NO_MSG((name_len > 0) &&
			(name_len < CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE));

		strcpy(data, name_ptr);
		*size = name_len;
		break;
	}
	default:
		LOG_WRN("Unknown config fetch option ID %" PRIu8, opt_id);
		break;
	}
}

static void set_config(const u8_t opt_id, const u8_t *data, const size_t size)
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
EVENT_SUBSCRIBE_FINAL(MODULE, config_event);
EVENT_SUBSCRIBE_FINAL(MODULE, config_fetch_request_event);
