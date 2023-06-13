/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>

#include "ui_input_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m_client, CONFIG_APP_LOG_LEVEL);

/**
 * @brief Callback used by the DK buttons and LEDs library.
 *
 * Submits a ui input event when a input device changes states.
 * The input device can either be a push-button or a on/off-switch.
 *
 * @param device_states Bitmask containing the devices' state,
 * i.e. bit 1 corresponds to the state of device 1 etc.
 * @param has_changed Bitmask indicating whether the devices' state has
 * changed, i.e. bit 1 high corresponds to a change in the state
 * of device 1.
 */
static void dk_input_device_event_handler(uint32_t device_states, uint32_t has_changed)
{
	uint8_t dev_num;

	while (has_changed) {
		dev_num = 0;

		/* Get bit position for next device that changed state. */
		for (uint8_t i = 0; i < 32; i++) {
			if (has_changed & BIT(i)) {
				dev_num = i + 1;
				break;
			}
		}

		if (dev_num == 0) {
			return;
		}

		/* Device number has been stored, remove from bitmask. */
		has_changed &= ~(1UL << (dev_num - 1));

		struct ui_input_event *event = new_ui_input_event();

		event->type = dev_num > 2 ? ON_OFF_SWITCH : PUSH_BUTTON;
		if (dev_num > 2) {
			event->device_number = (dev_num % 3) + 1;
		} else {
			event->device_number = dev_num;
		}
		event->state = (device_states & BIT(dev_num - 1));

		APP_EVENT_SUBMIT(event);
	}
}

int ui_input_init(void)
{
	static bool initialised;

	if (!initialised) {
		int ret = dk_buttons_init(dk_input_device_event_handler);

		if (ret) {
			LOG_ERR("Could not initialize buttons (%d)", ret);
			return ret;
		}

		initialised = true;
	}

	return 0;
}
