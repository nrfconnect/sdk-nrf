/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <helpers/nrfx_reset_reason.h>
#include <zephyr/storage/flash_map.h>

#define MODULE failsafe
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FAILSAFE_LOG_LEVEL);


static bool failsafe_check(void)
{
	uint32_t mask = NRFX_RESET_REASON_DOG_MASK |
			NRFX_RESET_REASON_LOCKUP_MASK;

	uint32_t reas = nrfx_reset_reason_get();

	return (reas & mask) != 0;
}

static void failsafe_erase(void)
{
	const struct flash_area *flash_area;
	int err = flash_area_open(FIXED_PARTITION_ID(storage_partition),
				  &flash_area);
	if (!err) {
		err = flash_area_erase(flash_area, 0,
				       FIXED_PARTITION_SIZE(storage_partition));
		flash_area_close(flash_area);
	}

	if (err) {
		LOG_ERR("Failsafe cannot erase settings");
	} else {
		LOG_WRN("Failsafe erased settings");
	}
}

static void failsafe_clear(void)
{
	nrfx_reset_reason_clear(nrfx_reset_reason_get());
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
				cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (failsafe_check()) {
				failsafe_erase();
			}
			failsafe_clear();

			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
