/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/stack.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/zbus/zbus.h>

#include "macros_common.h"
#include "fw_info_app.h"
#include "led.h"
#include "button_handler.h"
#include "button_assignments.h"
#include "nrfx_clock.h"
#include "sd_card.h"
#include "bt_mgmt.h"
#include "board_version.h"
#include "channel_assignment.h"
#include "streamctrl.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

static struct board_version board_rev;

#define ZBUS_ADD_OBS_TIMEOUT_MS K_MSEC(200)

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_CHAN_DECLARE(le_audio_chan);
ZBUS_CHAN_DECLARE(bt_mgmt_chan);
ZBUS_CHAN_DECLARE(volume_chan);
ZBUS_CHAN_DECLARE(cont_media_chan);

ZBUS_OBS_DECLARE(button_sub);
ZBUS_OBS_DECLARE(le_audio_evt_sub);
ZBUS_OBS_DECLARE(bt_mgmt_evt_sub);
ZBUS_OBS_DECLARE(volume_evt_sub);
ZBUS_OBS_DECLARE(content_control_evt_sub);

static int hfclock_config_and_start(void)
{
	int ret;

	/* Use this to turn on 128 MHz clock for cpu_app */
	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);

	ret -= NRFX_ERROR_BASE_NUM;
	if (ret) {
		return ret;
	}

	nrfx_clock_hfclk_start();
	while (!nrfx_clock_hfclk_is_running()) {
	}

	return 0;
}

static int zbus_init(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_ZBUS) && (CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE > 0)) {
		ret = zbus_chan_add_obs(&button_chan, &button_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to add button sub");
			return ret;
		}

		ret = zbus_chan_add_obs(&le_audio_chan, &le_audio_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to add le_audio sub");
			return ret;
		}

		ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to add bt_mgmt sub");
			return ret;
		}

		ret = zbus_chan_add_obs(&volume_chan, &volume_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to add volume sub");
			return ret;
		}

		ret = zbus_chan_add_obs(&cont_media_chan, &content_control_evt_sub,
					ZBUS_ADD_OBS_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to add content control sub");
			return ret;
		}
	}

	return 0;
}

static int leds_set(void)
{
	int ret;

	/* Blink LED 3 to indicate that APP core is running */
	ret = led_blink(LED_APP_3_GREEN);
	if (ret) {
		return ret;
	}

#if (CONFIG_AUDIO_DEV == HEADSET)
	enum audio_channel channel;

	channel_assignment_get(&channel);

	if (channel == AUDIO_CH_L) {
		ret = led_on(LED_APP_RGB, LED_COLOR_BLUE);
	} else {
		ret = led_on(LED_APP_RGB, LED_COLOR_MAGENTA);
	}

	if (ret) {
		return ret;
	}
#elif (CONFIG_AUDIO_DEV == GATEWAY)
	ret = led_on(LED_APP_RGB, LED_COLOR_GREEN);
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */

	return 0;
}

static int channel_assign_check(void)
{
#if (CONFIG_AUDIO_DEV == HEADSET) && CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME
	int ret;
	bool pressed;

	ret = button_pressed(BUTTON_VOLUME_DOWN, &pressed);
	if (ret) {
		return ret;
	}

	if (pressed) {
		channel_assignment_set(AUDIO_CH_L);
		return 0;
	}

	ret = button_pressed(BUTTON_VOLUME_UP, &pressed);
	if (ret) {
		return ret;
	}

	if (pressed) {
		channel_assignment_set(AUDIO_CH_R);
		return 0;
	}
#endif

	return 0;
}

int main(void)
{
	int ret;

	LOG_DBG("nRF5340 APP core started");

	ret = hfclock_config_and_start();
	ERR_CHK(ret);

	ret = led_init();
	ERR_CHK(ret);

	ret = button_handler_init();
	ERR_CHK(ret);

	channel_assignment_init();

	ret = channel_assign_check();
	ERR_CHK(ret);

	ret = fw_info_app_print();
	ERR_CHK(ret);

	ret = board_version_valid_check();
	ERR_CHK(ret);

	ret = board_version_get(&board_rev);
	ERR_CHK(ret);

	if (board_rev.mask & BOARD_VERSION_VALID_MSK_SD_CARD) {
		ret = sd_card_init();
		if (ret != -ENODEV) {
			ERR_CHK(ret);
		}
	}

	ret = zbus_init();
	ERR_CHK(ret);

	ret = bt_mgmt_init();
	ERR_CHK(ret);

	ret = leds_set();
	ERR_CHK(ret);

	ret = streamctrl_start();
	ERR_CHK(ret);
}
