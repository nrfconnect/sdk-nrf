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

#include <lvgl.h>

#include "tab_menu.h"
#include "tab_log.h"
#include "macros_common.h"

#define DISPLAY_THREAD_PRIORITY	  5
/* Must have lower priority than LVGL init */
#define DISPLAY_INIT_PRIORITY	  91
#define DISPLAY_THREAD_STACK_SIZE 1800

LOG_MODULE_REGISTER(display_log, CONFIG_DISPLAY_LOG_LEVEL);

static struct k_thread display_data;
static k_tid_t display_thread;

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_CHAN_DECLARE(le_audio_chan);

ZBUS_OBS_DECLARE(le_audio_evt_sub_display);

K_THREAD_STACK_DEFINE(display_thread_STACK, DISPLAY_THREAD_STACK_SIZE);

static void nrf5340_audio_dk_display_update_thread(void *arg1, void *arg2, void *arg3)
{
	uint64_t prev_tab_update = k_uptime_get();

	while (1) {
		if ((k_uptime_get() - prev_tab_update) >
		    CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_MENU_UPDATE_INTERVAL_MS) {
			tab_menu_update();
			prev_tab_update = k_uptime_get();
		}

		uint32_t time_till_next = lv_timer_handler();

		k_sleep(K_MSEC(time_till_next));
	}
}

static int nrf5340_audio_dk_display_init(void)
{
	int ret;
	const struct device *display_dev;
	lv_obj_t *tabview;
	lv_obj_t *tab_menu;
#ifdef CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG
	lv_obj_t *tab_log;
#endif /* CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG */

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display not ready");
		return -ENODEV;
	}

	ret = zbus_chan_add_obs(&le_audio_chan, &le_audio_evt_sub_display, K_MSEC(200));
	if (ret) {
		LOG_ERR("Failed to add ZBus observer (%d)", ret);
		return ret;
	}

	ret = display_blanking_off(display_dev);
	if (ret) {
		LOG_ERR("Failed to disable display blanking (%d)", ret);
		return ret;
	}

	tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

	tab_menu = lv_tabview_add_tab(tabview, "Menu");
	tab_menu_create(tab_menu);

#ifdef CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG
	tab_log = lv_tabview_add_tab(tabview, "Logging");
	tab_log_create(tab_log);
#endif /* CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG */

	lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);

	display_thread = k_thread_create(&display_data, display_thread_STACK,
					 K_THREAD_STACK_SIZEOF(display_thread_STACK),
					 nrf5340_audio_dk_display_update_thread, NULL, NULL, NULL,
					 K_PRIO_PREEMPT(DISPLAY_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(display_thread, "Nrf5340_audio__dk display thread");

	return 0;
}

SYS_INIT(nrf5340_audio_dk_display_init, APPLICATION, DISPLAY_INIT_PRIORITY);
