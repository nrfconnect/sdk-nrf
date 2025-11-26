/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "peripherals.h"

#include <nrfx_clock.h>

#include "led_assignments.h"
#include "led.h"
#include "button_handler.h"
#include "button_assignments.h"
#include "sd_card.h"
#include "nrf5340_audio_dk_version.h"
#include "device_location.h"

#include "sd_card_playback.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(peripherals, CONFIG_PERIPHERALS_LOG_LEVEL);

static struct board_version board_rev;

static int leds_set(void)
{
	int ret;

	/* Blink LED 3 to indicate that APP core is running */
	ret = led_blink(LED_AUDIO_APP_STATUS);
	if (ret) {
		return ret;
	}

#if (CONFIG_AUDIO_DEV == HEADSET)
	enum bt_audio_location location;

	device_location_get(&location);

	if (location == BT_AUDIO_LOCATION_FRONT_LEFT) {
		ret = led_on(LED_AUDIO_DEVICE_TYPE, LED_COLOR_BLUE);
	} else if (location == BT_AUDIO_LOCATION_FRONT_RIGHT) {
		ret = led_on(LED_AUDIO_DEVICE_TYPE, LED_COLOR_MAGENTA);
	} else {
		ret = led_on(LED_AUDIO_DEVICE_TYPE, LED_COLOR_WHITE);
	}
#elif (CONFIG_AUDIO_DEV == GATEWAY)
	ret = led_on(LED_AUDIO_DEVICE_TYPE, LED_COLOR_GREEN);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */

	if (ret) {
		return ret;
	}

	return 0;
}

static int location_assign_check(void)
{
#if (CONFIG_AUDIO_DEV == HEADSET) && CONFIG_DEVICE_LOCATION_SET_RUNTIME
	int ret;
	bool pressed_vol_down;
	bool pressed_vol_up;

	ret = button_pressed(BUTTON_VOLUME_DOWN, &pressed_vol_down);
	if (ret) {
		return ret;
	}

	ret = button_pressed(BUTTON_VOLUME_UP, &pressed_vol_up);
	if (ret) {
		return ret;
	}

	if (pressed_vol_down && pressed_vol_up) {
		device_location_set(BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT);
		return 0;
	}

	if (pressed_vol_down) {
		device_location_set(BT_AUDIO_LOCATION_FRONT_LEFT);
		return 0;
	}

	if (pressed_vol_up) {
		device_location_set(BT_AUDIO_LOCATION_FRONT_RIGHT);
		return 0;
	}
#endif

	return 0;
}

int peripherals_init(void)
{
	int ret;

	ret = led_init();
	if (ret) {
		LOG_ERR("Failed to initialize LED module");
		return ret;
	}

	ret = button_handler_init();
	if (ret) {
		LOG_ERR("Failed to initialize button handler");
		return ret;
	}

	ret = location_assign_check();
	if (ret) {
		LOG_ERR("Failed get location assignment");
		return ret;
	}

	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
		ret = nrf5340_audio_dk_version_valid_check();
		if (ret) {
			return ret;
		}

		ret = nrf5340_audio_dk_version_get(&board_rev);
		if (ret) {
			return ret;
		}

		if (board_rev.mask & BOARD_VERSION_VALID_MSK_SD_CARD) {
			ret = sd_card_init();
			if (ret != -ENODEV && ret != 0) {
				LOG_ERR("Failed to initialize SD card");
				return ret;
			}
		}

		if (IS_ENABLED(CONFIG_SD_CARD_PLAYBACK)) {
			ret = sd_card_playback_init();
			if (ret) {
				LOG_ERR("Failed to initialize SD card playback");
				return ret;
			}
		}
	}

	ret = leds_set();
	if (ret) {
		LOG_ERR("Failed to set LEDs");
		return ret;
	}

	/* Use this to turn on 128 MHz clock for cpu_app */
	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	ret -= NRFX_ERROR_BASE_NUM;
	if (ret) {
		return ret;
	}

	return 0;
}
