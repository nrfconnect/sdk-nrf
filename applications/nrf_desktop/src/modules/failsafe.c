/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/storage/flash_map.h>

#define MODULE failsafe
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FAILSAFE_LOG_LEVEL);


static int failsafe_check(bool *failure_detected)
{
	int err;
	uint32_t reas = 0;
	static const uint32_t mask = RESET_WATCHDOG | RESET_CPU_LOCKUP;

	err = hwinfo_get_reset_cause(&reas);
	if (err) {
		LOG_ERR("Failed to fetch reset cause: %d", err);
		return err;
	}

	*failure_detected = ((reas & mask) != 0);

	LOG_INF("Reset reason (0x%08X) trigger %s", reas,
		*failure_detected ? "active" : "inactive");

	return err;
}

static int failsafe_erase(void)
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
		LOG_ERR("Failsafe cannot erase settings: %d", err);
	} else {
		LOG_WRN("Failsafe erased settings");
	}

	return err;
}

static int failsafe_clear(void)
{
	int err;

	err = hwinfo_clear_reset_cause();
	if (err) {
		LOG_ERR("Failed to clear reset cause: %d", err);
	}

	return err;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
				cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err;
			bool failure_detected;
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			err = failsafe_check(&failure_detected);
			if (!err && failure_detected) {
				err = failsafe_erase();
			}

			if (!err) {
				err = failsafe_clear();
			}

			if (!err) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
