/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <lvgl.h>
#include "tab_menu.h"
#include <nrf5340_audio_common.h>
#include <button_assignments.h>
#include <errno.h>
#include "macros_common.h"
#include <pcm_stream_channel_modifier.h>
#include <hw_codec.h>
#include <audio_defines.h>
#include <channel_assignment.h>
#include "sw_codec_select.h"
#include "audio_datapath.h"

static lv_obj_t *device_info_label;
static lv_obj_t *device_mode_label;
static lv_obj_t *volume_level_label;
static lv_obj_t *play_pause_label;
static lv_obj_t *vu_bar;

static lv_style_t style_btn_gray;
static lv_style_t style_btn_red;

static enum audio_channel channel;

LOG_MODULE_DECLARE(display_log, CONFIG_DISPLAY_LOG_LEVEL);

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_SUBSCRIBER_DEFINE(le_audio_evt_sub_display, CONFIG_LE_AUDIO_MSG_SUB_QUEUE_SIZE);

static void *decoded_data;
static size_t *decoded_size;

static void style_init(lv_color_t style_color, lv_style_t *style_btn)
{
	lv_style_init(style_btn);
	lv_style_set_bg_color(style_btn, style_color);
	lv_style_set_bg_grad_color(style_btn, style_color);
}

static lv_obj_t *button_create(lv_obj_t *screen, char *label_text, lv_align_t button_position,
			       void (*button_callback_function)(lv_event_t *e),
			       const lv_font_t *font_size)
{
	lv_obj_t *button = lv_btn_create(screen);

	lv_obj_set_size(button, 47, 47);
	lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
	lv_obj_align(button, button_position, 0, 0);

	lv_obj_t *label = lv_label_create(button);

	lv_label_set_text(label, label_text);
	lv_obj_set_style_text_font(label, font_size, LV_STATE_DEFAULT);
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_event_cb(button, button_callback_function, LV_EVENT_CLICKED, NULL);

	lv_obj_add_style(button, &style_btn_red, LV_STATE_PRESSED);

	return label;
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

static void vu_bar_update(int vu_val)
{
	lv_bar_set_value(vu_bar, vu_val, LV_ANIM_ON);
}

static int streaming_state_update(uint8_t *streaming_state)
{
	int ret;
	const struct zbus_channel *chan;
	struct le_audio_msg msg;
	static uint8_t audio_streaming_event;

	ret = zbus_sub_wait(&le_audio_evt_sub_display, &chan, K_NO_WAIT);
	if (ret == -ENOMSG) {
		*streaming_state = audio_streaming_event;
		return 0;
	} else if (ret != 0) {
		LOG_ERR("zbus_sub_wait: %d", ret);
		return ret;
	}

	ret = zbus_chan_read(chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_ERR("zbus_chan_read: %d", ret);
		return ret;
	}

	audio_streaming_event = msg.event;
	*streaming_state = audio_streaming_event;

	return 0;
}

static void volume_label_update(void)
{
	uint32_t volume_level_int;
	int volume_level;
	/* The label will always be two digits, followed by an % and newline, giving length 4*/
	const size_t volume_label_length = 4;
	char volume_level_str[volume_label_length];

	volume_level_int = hw_codec_volume_get();
	volume_level = floor((float)volume_level_int * 100.0f / 128.0f);
	snprintf(volume_level_str, volume_label_length, "%2d%%", volume_level);
	lv_label_set_text(volume_level_label, volume_level_str);
}

/**
 * @brief Calculate average frame value for decoded audio buffer, in percent value
 *
 * @details Calculates the average of every frame in the decoded audio buffer kept in
 *          audio_datapath.c. The data buffer is statically kept in audio_datapath, so it is
 *          always valid when the device is streaming. NOTE! As the audio system is run in a thread
 *          with higher priority than the display thread there is a risk that the buffer is updated
 *          while calculation is ongoing.
 *
 * @retval int Frame value in percentage of possible max value
 */
static int calculate_vu_percentage(void)
{
	int frame_value_total = 0;
	int8_t bytes_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS / 8;

	char *pointer_input = (char *)decoded_data;

	/* Iterate through each sample */
	for (int i = 0; i < (*decoded_size / (bytes_per_sample)); i += 2) {
		if (channel == AUDIO_CH_L) {
			if (bytes_per_sample == 2) {
				frame_value_total += abs(*((int16_t *)pointer_input));
			} else if (bytes_per_sample == 4) {
				frame_value_total += abs(*((int32_t *)pointer_input));
			}

			pointer_input += 2 * bytes_per_sample;
		} else if (channel == AUDIO_CH_R) {
			/* Progress pointer first to read the right channel */
			pointer_input += 2 * bytes_per_sample;

			if (bytes_per_sample == 2) {
				frame_value_total += abs(*((int16_t *)pointer_input));
			} else if (bytes_per_sample == 4) {
				frame_value_total += abs(*((int32_t *)pointer_input));
			}
		} else {
			__ASSERT(false, "Unknown audio channel (%d)", channel);
		}
	}

	/* Calculate the max numeric value for a signed integer of bytes_per_sample number of
	 * bytes
	 */
	int frame_max_value = (((1 << ((8 * bytes_per_sample) - 2)) - 1) * 2) + 1;

	int frame_avg_val = frame_value_total / (int)(*decoded_size / 4);

	return round((double)frame_avg_val / (double)frame_max_value * 100);
}

static void headset_device_info_label_update(void)
{
	char *device_info;

	if (channel == AUDIO_CH_L) {
		device_info = "Headset Left";
	} else if (channel == AUDIO_CH_R) {
		device_info = "Headset Right";
	} else {
		device_info = "";
		__ASSERT(false, "Unknown audio channel (%d)", channel);
	}

	lv_label_set_text(device_info_label, device_info);
	lv_obj_align_to(device_info_label, device_mode_label, LV_ALIGN_OUT_TOP_MID, 0, 0);
}

void tab_menu_update(void)
{
	int ret;
	uint8_t streaming_state;

	ret = streaming_state_update(&streaming_state);
	if (ret) {
		LOG_ERR("Failed to update streaming state");
		return;
	}

	switch (streaming_state) {
	case LE_AUDIO_EVT_STREAMING:
		lv_label_set_text(play_pause_label, LV_SYMBOL_PAUSE);
		break;

	case LE_AUDIO_EVT_NOT_STREAMING:
		lv_label_set_text(play_pause_label, LV_SYMBOL_PLAY);
		break;
	default:
		break;
	}

	if (CONFIG_AUDIO_DEV == HEADSET) {
		enum audio_channel temp_channel;

		channel_assignment_get(&temp_channel);
		if (temp_channel != channel) {
			channel = temp_channel;
			headset_device_info_label_update();
		}

		volume_label_update();

		if (streaming_state == LE_AUDIO_EVT_STREAMING) {
			audio_datapath_buffer_ptr_get(&decoded_data, &decoded_size);
			vu_bar_update(calculate_vu_percentage());
		} else {
			vu_bar_update(0);
		}
	}
}

static void volume_label_create(lv_obj_t *screen)
{

	volume_level_label = lv_label_create(screen);

	lv_label_set_text(volume_level_label, "");
	lv_obj_align(volume_level_label, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void device_info_label_create(lv_obj_t *screen)
{
	char *mode;

	device_mode_label = lv_label_create(screen);
	device_info_label = lv_label_create(screen);

	if (IS_ENABLED(CONFIG_TRANSPORT_CIS)) {
		mode = "CIS";
	} else {
		mode = "BIS";
	}

	lv_label_set_text(device_mode_label, mode);
	lv_obj_align(device_mode_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	if (CONFIG_AUDIO_DEV == GATEWAY) {
		lv_label_set_text(device_info_label, "Gateway");
		lv_obj_align_to(device_info_label, device_mode_label, LV_ALIGN_OUT_TOP_MID, 0, 0);
	} else {
		headset_device_info_label_update();
	}
}

static void vu_bar_create(lv_obj_t *screen)
{
	static lv_style_t style_indic;

	lv_style_init(&style_indic);
	lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
	lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
	lv_style_set_bg_grad_color(&style_indic, lv_palette_main(LV_PALETTE_RED));
	lv_style_set_bg_grad_dir(&style_indic, LV_GRAD_DIR_HOR);

	vu_bar = lv_bar_create(screen);
	lv_obj_add_style(vu_bar, &style_indic, LV_PART_INDICATOR);
	lv_obj_set_size(vu_bar, 150, 20);
	lv_obj_center(vu_bar);
}

static void vu_bar_label_create(lv_obj_t *screen)
{
	lv_obj_t *vu_bar_label = lv_label_create(screen);

	lv_label_set_text(vu_bar_label, "VU-bar");
	lv_obj_align(vu_bar_label, LV_ALIGN_TOP_MID, 0, 20);
}

void tab_menu_create(lv_obj_t *screen)
{
	style_init(lv_palette_main(LV_PALETTE_RED), &style_btn_red);
	style_init(lv_palette_main(LV_PALETTE_GREY), &style_btn_gray);

	if (CONFIG_AUDIO_DEV == HEADSET) {
		volume_label_create(screen);
		vu_bar_create(screen);
		vu_bar_label_create(screen);

		lv_obj_t *change_stream_label =
			button_create(screen, LV_SYMBOL_BLUETOOTH, LV_ALIGN_TOP_LEFT,
				      btn4_button_event_cb, &lv_font_montserrat_20);

		if (IS_ENABLED(CONFIG_TRANSPORT_CIS) == 1) {
			lv_obj_remove_event_cb(lv_obj_get_parent(change_stream_label),
					       btn4_button_event_cb);
			lv_obj_add_style(lv_obj_get_parent(change_stream_label), &style_btn_gray,
					 LV_STATE_DEFAULT);
			lv_obj_add_style(lv_obj_get_parent(change_stream_label), &style_btn_gray,
					 LV_STATE_PRESSED);
		}
	}

	if (IS_ENABLED(CONFIG_AUDIO_TEST_TONE) && (CONFIG_AUDIO_DEV == GATEWAY)) {
		button_create(screen, "~", LV_ALIGN_TOP_LEFT, btn4_button_event_cb,
			      &lv_font_montserrat_48);
	}
	if (IS_ENABLED(CONFIG_AUDIO_MUTE)) {
		button_create(screen, LV_SYMBOL_MUTE, LV_ALIGN_BOTTOM_LEFT, btn5_button_event_cb,
			      &lv_font_montserrat_20);
	}

	play_pause_label = button_create(screen, LV_SYMBOL_PLAY, LV_ALIGN_LEFT_MID,
					 play_pause_button_event_cb, &lv_font_montserrat_14);
	button_create(screen, LV_SYMBOL_VOLUME_MAX, LV_ALIGN_TOP_RIGHT, volume_up_button_event_cb,
		      &lv_font_montserrat_20);
	button_create(screen, LV_SYMBOL_VOLUME_MID, LV_ALIGN_BOTTOM_RIGHT,
		      volume_down_button_event_cb, &lv_font_montserrat_20);
	device_info_label_create(screen);
}
