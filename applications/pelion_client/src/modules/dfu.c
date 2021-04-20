/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <storage/flash_map.h>
#include <pm_config.h>
#include <dfu/mcuboot.h>

#define MODULE dfu
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_DFU_LOG_LEVEL);

#include "pelion_event.h"


static int log_fw_version(void)
{
	if (!IS_ENABLED(CONFIG_LOG)) {
		return 0;
	}

	struct mcuboot_img_header header;

	int err = boot_read_bank_header(PM_MCUBOOT_PRIMARY_ID, &header,
					sizeof(header));

	if (!err) {
		LOG_INF("FW info: image size:%" PRIu32
			" major:%" PRIu8 " minor:%" PRIu8 " rev:0x%" PRIx16
			" build:0x%" PRIx32, header.h.v1.image_size,
			header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor,
			header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num);
	}

	return err;
}

static bool handle_pelion_state_event(const struct pelion_state_event *event)
{
	if (event->state == PELION_STATE_REGISTERED) {
		/* If we are able to register the currently running firmware
		 * may be confirmed safely.
		 */
		(void)boot_write_img_confirmed();
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

			if (log_fw_version()) {
				LOG_ERR("Cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (is_pelion_state_event(eh)) {
		return handle_pelion_state_event(cast_pelion_state_event(eh));
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, pelion_state_event);
