/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <hal/nrf_power.h>
#include <storage/flash_map.h>

#define MODULE failsafe
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FAILSAFE_LOG_LEVEL);


static bool failsafe_check(void)
{
	uint32_t mask = NRF_POWER_RESETREAS_DOG_MASK |
			NRF_POWER_RESETREAS_LOCKUP_MASK;

	uint32_t reas = nrf_power_resetreas_get(NRF_POWER);
	nrf_power_resetreas_clear(NRF_POWER, reas);

	return (reas & mask) != 0;
}

static void failsafe_erase(void)
{
	const struct flash_area *flash_area;
	int err = flash_area_open(FLASH_AREA_ID(storage), &flash_area);
	if (!err) {
		err = flash_area_erase(flash_area, 0, FLASH_AREA_SIZE(storage));
		flash_area_close(flash_area);
	}

	if (err) {
		LOG_ERR("Failsafe cannot erase settings");
	} else {
		LOG_WRN("Failsafe erased settings");
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
				cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (failsafe_check()) {
				failsafe_erase();
			}
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
