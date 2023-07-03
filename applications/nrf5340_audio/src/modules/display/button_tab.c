/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <display/display.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
#include <stdint.h>
#include "button_tab.h"
#include <C:\ncs\v2.4.0\nrf\applications\nrf5340_audio\src\modules\hw_codec.h>
#include <math.h>

LOG_MODULE_REGISTER(nrf5340audiodk_display, CONFIG_NRF5340AUDIODK_DISPLAY_LOG_LEVEL);

enum button_action {
	BUTTON_PRESS,
	BUTTON_ACTION_NUM,
};
struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};

ZBUS_CHAN_DECLARE(button_chan);

void button_create(lv_obj_t *current_screen, char label_text[20], lv_align_t *button_position,
		   void (*button_callback_function)(lv_event_t *e))
{
	lv_obj_t *button;
	lv_obj_t *label;

	button = lv_btn_create(current_screen);
	lv_obj_align(button, button_position, 0, 0);
	label = lv_label_create(button);
	lv_label_set_text(label, label_text);
	lv_obj_align(label, button_position, 0, 0);
	lv_obj_add_event_cb(button, button_callback_function, LV_EVENT_CLICKED, NULL);
}

static void play_pause_button_event_cb(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_PLAY_PAUSE;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}
int volume_level_int;

static void volume_up_button_event_cb(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_VOLUME_UP;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

static void volume_down_button_event_cb(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_VOLUME_DOWN;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}
static void btn4_button_event_cb(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_4;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

static void btn5_button_event_cb(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_5;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

void devicetype_label_create(lv_obj_t *current_screen)
{
	char what_dev_am_i[20];

#if (CONFIG_TRANSPORT_CIS)
#if (CONFIG_AUDIO_DEV == 1)
	strcpy(what_dev_am_i, "Headset\n CIS");
#endif
#if (CONFIG_AUDIO_DEV == 2)
	strcpy(what_dev_am_i, "Gateway\n CIS");
#endif
#endif
#if (CONFIG_TRANSPORT_BIS)
#if (CONFIG_AUDIO_DEV == 1)
	strcpy(what_dev_am_i, "Headset\n BIS");
#endif
#if (CONFIG_AUDIO_DEV == 2)
	strcpy(what_dev_am_i, "Gateway\n BIS");
#endif
#endif
	lv_obj_t *what_dev_am_i_label;
	what_dev_am_i_label = lv_label_create(current_screen);
	lv_label_set_text(what_dev_am_i_label, what_dev_am_i);
	lv_obj_align(what_dev_am_i_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}
extern void update_volume_level(void *arg1, void *arg2, void *arg3)
{
	lv_obj_t *button_tab = (lv_obj_t *)arg1;

	lv_obj_t *volume_level_label;
	int volume_level_int;
	int volume_level_prosent_int;
	char volume_level_prosent_str[8];

	volume_level_label = lv_label_create(button_tab);
	lv_label_set_text(volume_level_label, "Volume");
	lv_obj_align(volume_level_label, LV_ALIGN_RIGHT_MID, 0, 0);

	while (1) {
		volume_level_int = hw_codec_volume_get();
		/* volume_level_prosent_int = (volume_level_int / 128) * 100; */
		sprintf(volume_level_prosent_str, "%d", volume_level_int);
		lv_label_set_text(volume_level_label, volume_level_prosent_str);
		k_sleep(K_MSEC(1000));
	}
}

void button_tab_create(lv_obj_t *current_screen)
{

#if ((CONFIG_AUDIO_DEV == 1) && (CONFIG_TRANSPORT_BIS))
	button_create(current_screen, "Change audio stream", LV_ALIGN_TOP_LEFT,
		      btn4_button_event_cb);
#endif
#if ((CONFIG_AUDIO_DEV == 2) && (CONFIG_TRANSPORT_BIS))
	button_create(current_screen, "Test Tone", LV_ALIGN_TOP_LEFT, btn4_button_event_cb);
#endif
#if (CONFIG_TRANSPORT_CIS)
	button_create(current_screen, "Test Tone", LV_ALIGN_TOP_LEFT, btn4_button_event_cb);
#endif
	button_create(current_screen, "Play/Pause", LV_ALIGN_CENTER, play_pause_button_event_cb);
	button_create(current_screen, "Vol+", LV_ALIGN_TOP_RIGHT, volume_up_button_event_cb);
	button_create(current_screen, "Vol-", LV_ALIGN_BOTTOM_RIGHT, volume_down_button_event_cb);
	button_create(current_screen, "Mute", LV_ALIGN_BOTTOM_LEFT, btn5_button_event_cb);
	devicetype_label_create(current_screen);
}