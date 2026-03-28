/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "leds.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/sys/__assert.h>

#define LED_BLINK_MS	50
#define LED_BLINK_COUNT 10

LOG_MODULE_DECLARE(central_uart);

#if DT_HAS_CHOSEN(nordic_central_uart_ble_led)

static const struct gpio_dt_spec ble_led =
	GPIO_DT_SPEC_GET(DT_CHOSEN(nordic_central_uart_ble_led), gpios);
static bool ble_connected;
static uint8_t ble_led_blink_remaining;
static void ble_led_blink_handler(struct k_timer *timer);
static K_TIMER_DEFINE(ble_led_blink_timer, ble_led_blink_handler, NULL);

static void ble_led_blink_handler(struct k_timer *timer)
{
	int rc;

	if (ble_led_blink_remaining > 0) {
		ble_led_blink_remaining--;
		rc = gpio_pin_toggle_dt(&ble_led);
	} else {
		k_timer_stop(timer);
		rc = gpio_pin_set_dt(&ble_led, 1);
	}

	__ASSERT_NO_MSG(rc == 0);
}

void leds_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&ble_led)) {
		LOG_WRN("BLE LED not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&ble_led, GPIO_OUTPUT_INACTIVE);
	__ASSERT_NO_MSG(rc == 0);
}

void leds_set_ble_connected(bool connected)
{
	int rc;

	if (!gpio_is_ready_dt(&ble_led)) {
		return;
	}

	k_timer_stop(&ble_led_blink_timer);

	ble_connected = connected;
	rc = gpio_pin_set_dt(&ble_led, connected ? 1 : 0);
	__ASSERT_NO_MSG(rc == 0);
}

void leds_indicate_ble_traffic(void)
{
	if (!gpio_is_ready_dt(&ble_led) || !ble_connected) {
		return;
	}

	k_timer_stop(&ble_led_blink_timer);

	ble_led_blink_remaining = LED_BLINK_COUNT;
	k_timer_start(&ble_led_blink_timer, K_MSEC(LED_BLINK_MS), K_MSEC(LED_BLINK_MS));
}

#else

void leds_init(void)
{
	/* LED not configured */
}

void leds_set_ble_connected(bool connected)
{
	/* LED not configured */

	ARG_UNUSED(connected);
}

void leds_indicate_ble_traffic(void)
{
	/* LED not configured */
}

#endif /* DT_HAS_CHOSEN(nordic_central_uart_ble_led) */

#if DT_HAS_CHOSEN(nordic_central_uart_log_led)

/*
 * Implement a dummy logging backend that blinks the log LED on log activity.
 */
static const struct gpio_dt_spec log_led =
	GPIO_DT_SPEC_GET(DT_CHOSEN(nordic_central_uart_log_led), gpios);
static uint8_t log_led_blink_remaining;
static void log_led_blink_handler(struct k_timer *timer);
static K_TIMER_DEFINE(log_led_blink_timer, log_led_blink_handler, NULL);

static void log_led_blink_handler(struct k_timer *timer)
{
	int rc;

	if (log_led_blink_remaining > 0) {
		log_led_blink_remaining--;
		rc = gpio_pin_toggle_dt(&log_led);
	} else {
		k_timer_stop(timer);
		rc = gpio_pin_set_dt(&log_led, 0);
	}

	__ASSERT_NO_MSG(rc == 0);
}

static void log_backend_led_process(const struct log_backend *const backend,
				    union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(msg);

	k_timer_stop(&log_led_blink_timer);

	log_led_blink_remaining = LED_BLINK_COUNT;
	k_timer_start(&log_led_blink_timer, K_NO_WAIT, K_MSEC(LED_BLINK_MS));
}

static void log_backend_led_init(const struct log_backend *const backend)
{
	int rc;

	ARG_UNUSED(backend);

	if (!gpio_is_ready_dt(&log_led)) {
		LOG_WRN("Log LED not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&log_led, GPIO_OUTPUT_INACTIVE);
	__ASSERT_NO_MSG(rc == 0);
}

static const struct log_backend_api log_backend_led_api = {
	.process = log_backend_led_process,
	.init = log_backend_led_init,
};

LOG_BACKEND_DEFINE(log_backend_led, log_backend_led_api, true);
#endif /* DT_HAS_CHOSEN(nordic_central_uart_log_led) */
