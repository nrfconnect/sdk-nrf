#include <nrf5340audiodk_display/nrf5340audiodk_display.h>
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
#include "nrf5340audiodk_display_internal.h"

enum button_action {
	BUTTON_PRESS,
	BUTTON_ACTION_NUM,
};
struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};

ZBUS_CHAN_DECLARE(button_chan);

static void event_cb_play_pause_button(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_PLAY_PAUSE;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	/* 	if (ret) {
			LOG_ERR("Failed to publish button msg, ret: %d", ret);
	} */
}

static void event_cb_volume_up_button(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_VOLUME_UP;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	/* if (ret) {
		LOG_ERR("Failed to publish button msg, ret: %d", ret);
} */
}

static void event_cb_volume_down_button(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_VOLUME_DOWN;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	/* if (ret) {
			LOG_ERR("Failed to publish button msg, ret: %d", ret);
	} */
}
static void event_cb_btn4_button(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_4;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	/* 	if (ret) {
			LOG_ERR("Failed to publish button msg, ret: %d", ret);
	} */
}

static void event_cb_btn5_button(lv_event_t *e)
{
	struct button_msg msg;

	msg.button_action = BUTTON_PRESS;
	msg.button_pin = BUTTON_5;

	int ret;
	ret = zbus_chan_pub(&button_chan, &msg, K_NO_WAIT);
	/* 	if (ret) {
			LOG_ERR("Failed to publish button msg, ret: %d", ret);
	} */
}

void create_play_pause_button(lv_obj_t *current_screen)
{
	lv_obj_t *play_pause_button;
	lv_obj_t *play_pause_label;

	play_pause_button = lv_btn_create(current_screen);
	lv_obj_align(play_pause_button, LV_ALIGN_CENTER, 0, 0);
	play_pause_label = lv_label_create(play_pause_button);
	lv_label_set_text(play_pause_label, "Play/Pause");
	lv_obj_align(play_pause_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_event_cb(play_pause_button, event_cb_play_pause_button, LV_EVENT_CLICKED, NULL);
}

void create_volume_buttons(lv_obj_t *current_screen)
{
	lv_obj_t *volume_up_button;
	lv_obj_t *volume_down_button;

	lv_obj_t *volume_up_label;
	lv_obj_t *volume_down_label;
	lv_obj_t *volume_level_label;

	volume_up_button = lv_btn_create(current_screen);
	volume_down_button = lv_btn_create(current_screen);

	volume_up_label = lv_label_create(volume_up_button);
	volume_down_label = lv_label_create(volume_down_button);
	volume_level_label = lv_label_create(current_screen);

	lv_label_set_text(volume_up_label, "Vol+");
	lv_label_set_text(volume_down_label, "Vol-");
	lv_label_set_text(volume_level_label, "Volume");

	lv_obj_align(volume_up_button, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_align(volume_down_button, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

	lv_obj_align(volume_up_label, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_align(volume_down_label, LV_ALIGN_CENTER, 0, 0);
	lv_obj_align(volume_level_label, LV_ALIGN_TOP_RIGHT, 0, 40);
	lv_obj_add_event_cb(volume_up_button, event_cb_volume_up_button, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(volume_down_button, event_cb_volume_down_button, LV_EVENT_CLICKED,
			    NULL);
}

void create_btn4_gateway_button(lv_obj_t *current_screen)
{
	lv_obj_t *btn4_button;
	lv_obj_t *btn4_label;

	btn4_button = lv_btn_create(current_screen);
	lv_obj_align(btn4_button, LV_ALIGN_TOP_LEFT, 0, 0);
	btn4_label = lv_label_create(btn4_button);
	lv_label_set_text(btn4_label, "Test tone");
	lv_obj_align(btn4_label, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_add_event_cb(btn4_button, event_cb_btn4_button, LV_EVENT_CLICKED, NULL);
}
void create_btn4_headset_button(lv_obj_t *current_screen)
{
	lv_obj_t *btn4_button;
	lv_obj_t *btn4_label;

	btn4_button = lv_btn_create(current_screen);
	lv_obj_align(btn4_button, LV_ALIGN_TOP_LEFT, 0, 0);
	btn4_label = lv_label_create(btn4_button);
	lv_label_set_text(btn4_label, "Change audio stream");
	lv_obj_align(btn4_label, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_add_event_cb(btn4_button, event_cb_btn4_button, LV_EVENT_CLICKED, NULL);
}
void create_btn5_button(lv_obj_t *current_screen)
{
	lv_obj_t *btn5_button;
	lv_obj_t *btn5_label;

	btn5_button = lv_btn_create(current_screen);
	lv_obj_align(btn5_button, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	btn5_label = lv_label_create(btn5_button);
	lv_label_set_text(btn5_label, "Mute");
	lv_obj_align(btn5_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	lv_obj_add_event_cb(btn5_button, event_cb_btn5_button, LV_EVENT_CLICKED, NULL);
}
void create_devicetype_label(lv_obj_t *current_screen)
{
	char what_dev_am_i[8];

#if (CONFIG_AUDIO_DEV == 1)
	strcpy(what_dev_am_i, "Headset");
#endif
#if (CONFIG_AUDIO_DEV == 2)
	strcpy(what_dev_am_i, "Gateway");
#endif
	lv_obj_t *what_dev_am_i_label;
	what_dev_am_i_label = lv_label_create(current_screen);
	lv_label_set_text(what_dev_am_i_label, what_dev_am_i);
	lv_obj_align(what_dev_am_i_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}
void create_button_tab(lv_obj_t *screen)
{
#if (CONFIG_AUDIO_DEV == 1)
	create_btn4_headset_button(screen);
#endif
#if (CONFIG_AUDIO_DEV == 2)
	create_btn4_gateway_button(screen);
#endif
	create_play_pause_button(screen);
	create_volume_buttons(screen);
	create_btn5_button(screen);
	create_devicetype_label(screen);
}