/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "display.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include "button_tab.h"
#include <math.h>
#include <nrf5340_audio_common.h>
#include <button_assignments.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(display, CONFIG_DISPLAY_LOG_LEVEL);

ZBUS_CHAN_DECLARE(button_chan);

static void button_create(lv_obj_t *current_screen, char *label_text, lv_align_t button_position,
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
	int ret;
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_PLAY_PAUSE;

	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

static void volume_up_button_event_cb(lv_event_t *e)
{
	int ret;
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_VOLUME_UP;

	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

static void volume_down_button_event_cb(lv_event_t *e)
{
	int ret;
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_VOLUME_DOWN;

	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}
static void btn4_button_event_cb(lv_event_t *e)
{
	int ret;
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_4;

	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

static void btn5_button_event_cb(lv_event_t *e)
{
	int ret;
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_5;

	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
	}
}

static void devicetype_label_create(lv_obj_t *current_screen)
{
	char *what_dev_am_i;

#if (CONFIG_TRANSPORT_CIS)
#if (CONFIG_AUDIO_DEV == 1)
	what_dev_am_i = "Headset\n CIS";
#endif
#if (CONFIG_AUDIO_DEV == 2)
	what_dev_am_i = "Gateway\n CIS";
#endif
#endif
#if (CONFIG_TRANSPORT_BIS)
#if (CONFIG_AUDIO_DEV == 1)
	what_dev_am_i = "Headset\n BIS";
#endif
#if (CONFIG_AUDIO_DEV == 2)
	what_dev_am_i = "Gateway\n BIS";
#endif
#endif
	lv_obj_t *what_dev_am_i_label;
	what_dev_am_i_label = lv_label_create(current_screen);
	lv_label_set_text(what_dev_am_i_label, what_dev_am_i);
	lv_obj_align(what_dev_am_i_label, LV_ALIGN_BOTTOM_MID, 0, 0);
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
	button_create(current_screen, "Play/Pause", LV_ALIGN_LEFT_MID, play_pause_button_event_cb);
	button_create(current_screen, "Vol+", LV_ALIGN_TOP_RIGHT, volume_up_button_event_cb);
	button_create(current_screen, "Vol-", LV_ALIGN_BOTTOM_RIGHT, volume_down_button_event_cb);
	button_create(current_screen, "Mute", LV_ALIGN_BOTTOM_LEFT, btn5_button_event_cb);
	devicetype_label_create(current_screen);
}