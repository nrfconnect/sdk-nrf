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
#include <zephyr/logging/log_output.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <lvgl.h>
#include "tab_log.h"

#define LOG_MSG_MAX_LENGTH	       200
#define LOG_MSG_N_TIMESTAMP_MAX_LENGTH 220

LOG_MODULE_DECLARE(display_log, CONFIG_DISPLAY_LOG_LEVEL);

/* This is used so there are no conflicts between scheduler and LVGL */
K_MUTEX_DEFINE(nrf5340_display_mtx);

struct backend_context {
	uint32_t cnt;
	const char *exp_str[10];
	uint32_t delay_ms;
	bool active;
	struct k_timer timer;
};
static struct backend_context display_log_context;

static lv_obj_t *tab_log;
static int log_list_length;
static bool stop_logging;

/**
 * @brief  putc implementation for logging backend process
 *
 * @details See documentation for cbprint_cb function pointer in zephyr\sys\cbprintf.h
 */
static int cbprintf_callback(int c, void *ctx)
{
	char **p = ctx;

	**p = c;
	(*p)++;
	return c;
}

static void log_panic_handler(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static void expire_cb(struct k_timer *timer)
{
	void *ctx = k_timer_user_data_get(timer);
	struct backend_context *context = (struct backend_context *)ctx;

	context->active = true;
}

static void backend_init(const struct log_backend *const backend)
{
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;

	k_timer_init(&context->timer, expire_cb, NULL);
	k_timer_user_data_set(&context->timer, (void *)context);
	k_timer_start(&context->timer, K_MSEC(context->delay_ms), K_NO_WAIT);
}

static int backend_is_ready(const struct log_backend *const backend)
{
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;

	return context->active ? 0 : -EBUSY;
}

static void add_log_msg_to_log_list(lv_obj_t *tab_log, char *log_msg, lv_color_t log_msg_color)
{
	lv_obj_t *label = lv_label_create(tab_log);

	lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(label, 290);
	lv_label_set_text(label, log_msg);
	lv_obj_set_style_text_color(label, log_msg_color, 0);
	lv_obj_scroll_to_view_recursive(label, LV_ANIM_OFF);
}

static void backend_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	log_timestamp_t msg_timestamp;
	int msg_timestamp_in_us;
	char str[LOG_MSG_MAX_LENGTH];
	char timestamp_n_str[LOG_MSG_N_TIMESTAMP_MAX_LENGTH];
	char *pstr = str;

	size_t len;

	uint8_t *p = log_msg_get_package(&msg->log, &len);

	int slen = cbpprintf(cbprintf_callback, &pstr, p);

	str[slen] = '\0';

	if (IS_ENABLED(CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG_TIMESTAMP)) {
		msg_timestamp = log_msg_get_timestamp(&msg->log);
		msg_timestamp_in_us = log_output_timestamp_to_us(msg_timestamp);
		snprintf(timestamp_n_str, sizeof(timestamp_n_str), "[%d] %s", msg_timestamp_in_us,
			 str);
		strcpy(str, timestamp_n_str);
	}

	k_mutex_lock(&nrf5340_display_mtx, K_FOREVER);

	switch (log_msg_get_level(&msg->log)) {
	case LOG_LEVEL_ERR:
		if (CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG_LEVEL >= LOG_LEVEL_ERR) {
			log_list_length++;
			add_log_msg_to_log_list(tab_log, str,
						lv_palette_lighten(LV_PALETTE_RED, 1));
		}

		break;

	case LOG_LEVEL_WRN:
		if (CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG_LEVEL >= LOG_LEVEL_WRN) {
			log_list_length++;
			add_log_msg_to_log_list(tab_log, str,
						lv_palette_lighten(LV_PALETTE_YELLOW, 1));
		}

		break;

	case LOG_LEVEL_INF:
		if (CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG_LEVEL >= LOG_LEVEL_INF) {
			log_list_length++;
			add_log_msg_to_log_list(tab_log, str,
						lv_palette_lighten(LV_PALETTE_GREY, 5));
		}

		break;

	case LOG_LEVEL_DBG:
		if (CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG_LEVEL >= LOG_LEVEL_DBG) {
			log_list_length++;
			add_log_msg_to_log_list(tab_log, str,
						lv_palette_lighten(LV_PALETTE_GREY, 5));
		}

		break;

	default:
		break;
	}

	if (log_list_length >= CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG_MAX_LEN) {
		lv_obj_t *oldest_label = lv_obj_get_child(tab_log, 0);

		lv_group_remove_obj(oldest_label);
		lv_obj_del(oldest_label);
		log_list_length--;
	}

	k_mutex_unlock(&nrf5340_display_mtx);
}

static const struct log_backend_api backend_api = {.process = backend_process,
						   .init = backend_init,
						   .is_ready = backend_is_ready,
						   .panic = log_panic_handler};

LOG_BACKEND_DEFINE(display_log_backend, backend_api, false, &display_log_context);

static void tab_log_long_press_cb(lv_event_t *e)
{
	if (stop_logging) {
		log_backend_activate(&display_log_backend, &display_log_context);
		add_log_msg_to_log_list(tab_log, "Logging backend activated",
					lv_palette_lighten(LV_PALETTE_YELLOW, 5));
		stop_logging = false;
	} else if (!stop_logging) {
		add_log_msg_to_log_list(tab_log, "Logging backend deactivated",
					lv_palette_lighten(LV_PALETTE_YELLOW, 5));
		log_backend_deactivate(&display_log_backend);
		stop_logging = true;
	}
}

void tab_log_create(lv_obj_t *screen)
{
	stop_logging = false;
	tab_log = screen;
	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_bg_color(screen, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

	lv_obj_add_event_cb(screen, tab_log_long_press_cb, LV_EVENT_LONG_PRESSED, NULL);
	display_log_context.delay_ms = 0;

	log_backend_activate(&display_log_backend, &display_log_context);
}
