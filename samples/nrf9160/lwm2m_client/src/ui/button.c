/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include "ui.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(button, CONFIG_UI_LOG_LEVEL);

static ui_callback_t callback;

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	struct ui_evt evt;
	uint8_t btn_num;

	while (has_changed) {
		btn_num = 0;

		/* Get bit position for next button that changed state. */
		for (uint8_t i = 0; i < 32; i++) {
			if (has_changed & BIT(i)) {
				btn_num = i + 1;
				break;
			}
		}

		/* Button number has been stored, remove from bitmask. */
		has_changed &= ~(1UL << (btn_num - 1));

		evt.button = btn_num;
		evt.type = (button_states & BIT(btn_num - 1))
				? UI_EVT_BUTTON_ACTIVE
				: UI_EVT_BUTTON_INACTIVE;

		callback(&evt);
	}
}

int ui_button_init(ui_callback_t cb)
{
	int err = 0;

	if (cb) {
		callback  = cb;

		err = dk_buttons_init(button_handler);
		if (err) {
			LOG_ERR("Could not initialize buttons, err code: %d\n",
				err);
			return err;
		}
	}

	return err;
}

bool ui_button_is_active(uint32_t button)
{
	return dk_get_buttons() & BIT((button - 1));
}
