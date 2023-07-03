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
#include <C:\ncs\v2.4.0\nrf\applications\nrf5340_audio\src\modules\hw_codec.h>
#include <math.h>
#include <zephyr/logging/log_output.h>
#include "log_tab.h"
#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(display, CONFIG_DISPLAY_LOG_LEVEL);

lv_obj_t *wrn_label;

struct backend_context {
	uint32_t cnt;
	const char *exp_str[10];
	uint32_t delay;
	bool active;
	struct k_timer timer;
};
static int cbprintf_callback(int c, void *ctx)
{
	char **p = ctx;

	**p = c;
	(*p)++;

	return c;
}
static void panic(const struct log_backend *const backend)
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
	k_timer_start(&context->timer, K_MSEC(context->delay), K_NO_WAIT);
}
static int backend_is_ready(const struct log_backend *const backend)
{
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;

	return context->active ? 0 : -EBUSY;
}
static void backend_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	char str[100];
	char *pstr = str;
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;
	size_t len;
	uint8_t *p = log_msg_get_package(&msg->log, &len);

	(void)len;
	int slen = cbpprintf(cbprintf_callback, &pstr, p);

	str[slen] = '\0';
	if (log_msg_get_level(&msg->log) == LOG_LEVEL_WRN) {
		LOG_INF("Philip");
		lv_label_set_text(wrn_label, str);
	}
}
static const struct log_backend_api backend_api = {.process = backend_process,
						   .init = backend_init,
						   .is_ready = backend_is_ready,
						   .panic = panic};
void log_tab_create(lv_obj_t *screen)
{
	wrn_label = lv_label_create(screen);
	lv_label_set_text(wrn_label, "PHILIP");
	lv_obj_align(wrn_label, LV_ALIGN_BOTTOM_MID, 0, 0);

	static struct backend_context context1 = {.delay = 500};
	LOG_BACKEND_DEFINE(philip_backend, backend_api, true, &context1);
}