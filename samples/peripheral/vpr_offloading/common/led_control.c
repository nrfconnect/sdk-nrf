/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "ipc_comm.h"
#include "led_control.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_ctrl);

/* Module implements local execution of the LED control API. If code is complied
 * on VPR then module has support for IPC communication as control comes from the
 * application core.
 */
#ifdef CONFIG_RISCV_CORE_NORDIC_VPR
#define REMOTE_SUPPORT 1
#endif

struct led_control_data {
#ifdef REMOTE_SUPPORT
	struct ipc_comm_data ipc;
#endif
	struct k_timer timer;
	struct led_control_msg_start params;
	bool is_on;
};

struct led_control_config {
#ifdef REMOTE_SUPPORT
	struct ipc_comm ipc;
#endif
	struct gpio_dt_spec led;
};

struct led_control {
	const struct led_control_config *config;
	struct led_control_data *data;
};

static void change_led_state(const struct led_control *ledc)
{
	uint32_t next_period;
	uint32_t val;
	int rv;

	if (ledc->data->is_on) {
		next_period = ledc->data->params.period_off;
		val = 0;
	} else {
		next_period = ledc->data->params.period_on;
		val = 1;
	}

	ledc->data->is_on = !ledc->data->is_on;
	rv = gpio_pin_set_dt(&ledc->config->led, val);
	(void)rv;
	__ASSERT_NO_MSG(rv >= 0);
	k_timer_start(&ledc->data->timer, K_MSEC(next_period), K_NO_WAIT);
}

static void led_start(const struct led_control *ledc, uint32_t period_on, uint32_t period_off)
{
	ledc->data->params.period_on = period_on;
	ledc->data->params.period_off = period_off;
	change_led_state(ledc);
}

static void led_stop(const struct led_control *ledc)
{
	int rv;

	k_timer_stop(&ledc->data->timer);
	rv = gpio_pin_set_dt(&ledc->config->led, 0);
	ledc->data->is_on = false;

	(void)rv;
	__ASSERT_NO_MSG(rv >= 0);
}

static void timeout_handle(struct k_timer *timer)
{
	change_led_state((struct led_control *)k_timer_user_data_get(timer));
}

#ifdef REMOTE_SUPPORT
static void ipc_comm_cb(void *instance, const void *buf, size_t len)
{
	const struct led_control *ledc = instance;
	const struct led_control_msg *msg = buf;

	switch (msg->type) {
	case LED_CONTROL_MSG_START:
		led_start(ledc, msg->start.period_on, msg->start.period_off);
		break;
	case LED_CONTROL_MSG_STOP:
		led_stop(ledc);
		break;
	default:
		break;
	}
}
#endif

static struct led_control_data data;
static const struct led_control_config config;
static const struct led_control led_ctrl = {.data = &data, .config = &config};
static const struct led_control_config config = {
#ifdef REMOTE_SUPPORT
	.ipc = IPC_COMM_INIT(LED_CONTROL_EP_NAME, ipc_comm_cb, &led_ctrl,
			     DEVICE_DT_GET(DT_ALIAS(remote_led_control_ipc)), &data.ipc,
			     &config.ipc),
#endif
	.led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
};

static int led_control_init(void)
{
	const struct led_control *ledc = &led_ctrl;
	int rv;

	k_timer_init(&ledc->data->timer, timeout_handle, NULL);
	k_timer_user_data_set(&ledc->data->timer, (void *)ledc);

	rv = gpio_pin_configure_dt(&ledc->config->led, GPIO_OUTPUT);
	if (rv < 0) {
		return rv;
	}

	rv = gpio_pin_set_dt(&ledc->config->led, 0);
	if (rv < 0) {
		return rv;
	}

#ifdef REMOTE_SUPPORT
	return ipc_comm_init(&ledc->config->ipc);
#else
	return 0;
#endif
}

int led_control_start(uint32_t period_on, uint32_t period_off)
{
	led_start(&led_ctrl, period_on, period_off);
	return 0;
}

int led_control_stop(void)
{
	led_stop(&led_ctrl);
	return 0;
}

SYS_INIT(led_control_init, APPLICATION, 0);
