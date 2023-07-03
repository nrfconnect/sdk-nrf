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
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include "log_tab.h"
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(display, CONFIG_DISPLAY_LOG_LEVEL);

/* static void panic(const struct log_backend *const backend)
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
static const struct log_backend_api backend_api = {.process = backend_process,
						   .init = backend_init,
						   .is_ready = backend_is_ready,
						   .panic = panic};
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

	zassert_equal(strcmp(str, context->exp_str[context->cnt]), 0,
		      "Unexpected string %s (exp:%s)", str, context->exp_str[context->cnt]);

	context->cnt++;
}

void log_tab_create()
{
} */