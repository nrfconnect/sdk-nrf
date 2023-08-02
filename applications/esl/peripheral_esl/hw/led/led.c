/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/led.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>

#include "esl.h"

LOG_MODULE_DECLARE(peripheral_esl);

#if IS_ENABLED(CONFIG_LED_PWM)
#if DT_NODE_HAS_STATUS(DT_INST(0, pwm_leds), okay)
#define LED_PWM_NODE_ID	 DT_INST(0, pwm_leds)
#define LED_PWM_DEV_NAME DEVICE_DT_NAME(LED_PWM_NODE_ID)
#else
#error "No LED PWM device found"
#endif /* (DT_NODE_HAS_STATUS pwm_leds) */

#define LED_PWM_LABEL(led_node_id) DT_PROP_OR(led_node_id, label, NULL),

const char *led_label[] = {DT_FOREACH_CHILD(LED_PWM_NODE_ID, LED_PWM_LABEL)};
const struct device *led_pwm;

int led_init(void)
{
	led_pwm = device_get_binding(LED_PWM_DEV_NAME);
	if (led_pwm) {
		LOG_INF("Found device %s", LED_PWM_DEV_NAME);
	} else {
		LOG_ERR("Device %s not found", LED_PWM_DEV_NAME);
		return -ENOENT;
	}

	return 0;
}
void led_control(uint8_t led_idx, uint8_t color_brightness, bool onoff)
{
	int err;
	uint8_t level = (((color_brightness & 0xC0) >> ESL_LED_BRIGHTNESS_BIT) + 1) * 25;

	if (onoff) {
		err = led_set_brightness(led_pwm, led_idx, level);
	} else {
		err = led_off(led_pwm, led_idx);
	}
	if (err) {
		LOG_ERR("led_set_brightness err (%d)", err);
	}
}
#else
static uint8_t esl_leds[] = {DK_LED1, DK_LED2, DK_LED3, DK_LED4};
int led_init(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
		return err;
	}

	return 0;
}
void led_control(uint8_t led_idx, uint8_t color_brightness, bool onoff)
{
	uint8_t level = (((color_brightness & 0xC0) >> ESL_LED_BRIGHTNESS_BIT) + 1) * 25;

	LOG_DBG("LED %d %s level %d", led_idx, (onoff == 1) ? "on" : "off", level);

	dk_set_led(esl_leds[led_idx], onoff);
}
#endif /* CONFIG_LED_PWM */
