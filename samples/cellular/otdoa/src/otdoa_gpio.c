/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

#include "otdoa_sample_app.h"
#include "otdoa_gpio.h"

#define OTDOA_LED    DK_LED1 /* Red LED on thingy91X, green LED1 on 9161dk */
#define OTDOA_BUTTON DK_BTN1

static bool led_is_init;
static volatile bool led_state;
static volatile BLINK_SPEED led_mode = NONE;
struct k_timer led_timer;

/* forward reference */
static void button_cb(uint32_t button_state, uint32_t has_changed);

LOG_MODULE_DECLARE(otdoa_sample, LOG_LEVEL_INF);

static void led_timer_cb(struct k_timer *timer)
{
	toggle_led();

	if (led_mode == FLASH) {
		/* need to manually trigger the next shot, due to different timing */
		k_timer_start(timer, led_state ? K_MSEC(100) : K_MSEC(5000), K_NO_WAIT);
	}
}

int init_led(void)
{

	if (led_is_init) {
		return 0;
	}

	/* Use library button & LED functions */
	dk_leds_init();

	k_timer_init(&led_timer, led_timer_cb, NULL);

	led_is_init = true;

	return 0;
}

int set_led(int state)
{
	led_state = state;
	return dk_set_led(OTDOA_LED, led_state);
}

int toggle_led(void)
{
	led_state = !led_state;
	return dk_set_led(OTDOA_LED, led_state);
}

void set_blink_mode(BLINK_SPEED speed)
{
	if (!led_is_init) {
		if (init_led()) {
			LOG_ERR("Failed to initialize LED!");
			return;
		}
	}
	led_mode = speed;
	LOG_DBG("Setting blink mode to %d", speed);
	switch (speed) {
	case SLOW:
		k_timer_start(&led_timer, K_MSEC(2000), K_MSEC(2000));
		break;
	case FLASH:
		/* this is a oneshot timer because the delay needs to change */
		k_timer_start(&led_timer, led_state ? K_MSEC(100) : K_MSEC(5000), K_NO_WAIT);
		break;
	case FAST:
		k_timer_start(&led_timer, K_MSEC(250), K_MSEC(250));
		break;
	case MED:
		k_timer_start(&led_timer, K_MSEC(500), K_MSEC(500));
		break;
	case NONE:
		set_led(false);
		k_timer_stop(&led_timer);
		break;
	default:
		printk("Unknown LED blink mode %d\n", led_mode);
		break;
	}
}

void set_blink_sleep(void)
{
	set_blink_mode(FLASH);
}

void set_blink_prs(void)
{
	set_blink_mode(FAST);
}

void set_blink_download(void)
{
	set_blink_mode(MED);
}

void set_blink_error(void)
{
	set_blink_mode(SLOW);
}

/*
 *  Buttons
 */
static void button_cb(uint32_t button_state, uint32_t has_changed)
{
	uint32_t button_mask = BIT(OTDOA_BUTTON);

	if (has_changed & button_mask) {
		if (button_state & button_mask) {
			LOG_INF("Button high");
		} else {
			LOG_INF("Button low");
			/* start a test for 32 PRS occasions.  Timeout = 120 seconds */
			otdoa_sample_start(32, 0, OTDOA_SAMPLE_DEFAULT_TIMEOUT_MS);
		}
	}
}

int init_button(void)
{
	dk_buttons_init(button_cb);
	return 0;
}
