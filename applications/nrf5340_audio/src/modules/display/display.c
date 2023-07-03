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

#define MY_PRIORITY 5
LOG_MODULE_REGISTER(app);

int test;
enum button_action {
	BUTTON_PRESS,
	BUTTON_ACTION_NUM,
};
struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};

ZBUS_CHAN_DECLARE(button_chan);

void create_timestamp_label(lv_obj_t *current_screen)
{
	static uint32_t count;
	char count_str[11] = {0};
	lv_obj_t *count_label;
	count_label = lv_label_create(current_screen);
	lv_obj_align(count_label, LV_ALIGN_BOTTOM_MID, 0, 0);
	while (1) {
		if ((count % 100) == 0U) {
			sprintf(count_str, "%d", count / 100U);
			lv_label_set_text(count_label, count_str);
		}
		lv_task_handler();
		++count;
		k_sleep(K_MSEC(10));
	}
}
K_THREAD_STACK_DEFINE(update_volume_level_thread_STACK, CONFIG_MAIN_STACK_SIZE);

void display_init()
{
	struct k_thread update_volume_level_data;
	k_tid_t update_volume_level_thread;
	lv_obj_t *tabview;
	tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

	lv_obj_t *button_tab = lv_tabview_add_tab(tabview, "Button Tab");
	lv_obj_t *log_tab = lv_tabview_add_tab(tabview, "Log Tab");
	lv_obj_t *state_tab = lv_tabview_add_tab(tabview, "State Tab");

	const struct device *display_dev;
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}
	update_volume_level_thread = k_thread_create(
		&update_volume_level_data, update_volume_level_thread_STACK,
		K_THREAD_STACK_SIZEOF(update_volume_level_thread_STACK), update_volume_level,
		(void *)button_tab, NULL, NULL, MY_PRIORITY, 0, K_NO_WAIT);

	display_blanking_off(display_dev);
	button_tab_create(button_tab);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(16));
	}
}