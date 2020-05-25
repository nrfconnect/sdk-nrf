/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>

#include "ui.h"
#include "buzzer.h"
#include "led_pwm.h"
#include "nmos.h"

LOG_MODULE_REGISTER(ui, CONFIG_UI_LOG_LEVEL);

static enum ui_led_pattern current_led_state;
static ui_callback_t callback;

#if !defined(CONFIG_UI_LED_USE_PWM)
static struct k_delayed_work leds_update_work;

/**@brief Update LEDs state. */
static void leds_update(struct k_work *work)
{
	static bool led_on;
	static u8_t current_led_on_mask;
	u8_t led_on_mask;

	led_on_mask = UI_LED_GET_ON(current_led_state);
	led_on = !led_on;

	if (led_on) {
		led_on_mask |= UI_LED_GET_BLINK(current_led_state);
	} else {
		led_on_mask &= ~UI_LED_GET_BLINK(current_led_state);
	}

	if (led_on_mask != current_led_on_mask) {
#if defined(CONFIG_DK_LIBRARY)
		dk_set_leds(led_on_mask);
#endif
		current_led_on_mask = led_on_mask;
	}

	if (work) {
		if (led_on) {
			k_delayed_work_submit(&leds_update_work,
					      K_MSEC(UI_LED_ON_PERIOD_NORMAL));
		} else {
			k_delayed_work_submit(&leds_update_work,
					      K_MSEC(UI_LED_OFF_PERIOD_NORMAL));
		}
	}
}
#endif /* CONFIG_UI_LED_USE_PWM */

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(u32_t button_states, u32_t has_changed)
{
	struct ui_evt evt;
	u8_t btn_num;

	while (has_changed) {
		btn_num = 0;

		/* Get bit position for next button that changed state. */
		for (u8_t i = 0; i < 32; i++) {
			if (has_changed & BIT(i)) {
				btn_num = i + 1;
				break;
			}
		}

		/* Button number has been stored, remove from bitmask. */
		has_changed &= ~(1UL << (btn_num - 1));

		evt.button = btn_num;
		evt.type = (button_states & BIT(btn_num - 1))
				? UI_EVT_BUTTON_ACTIVE
				: UI_EVT_BUTTON_INACTIVE;

		callback(evt);
	}
}

void ui_led_set_pattern(enum ui_led_pattern state)
{
	current_led_state = state;
#ifdef CONFIG_UI_LED_USE_PWM
	ui_led_set_effect(state);
#else
	current_led_state = state;
#endif /* CONFIG_UI_LED_USE_PWM */
}

enum ui_led_pattern ui_led_get_pattern(void)
{
	return current_led_state;
}

int ui_led_set_color(u8_t red, u8_t green, u8_t blue)
{
#ifdef CONFIG_UI_LED_USE_PWM
	return ui_led_set_rgb(red, green, blue);
#else
	return -ENOTSUP;
#endif /* CONFIG_UI_LED_USE_PWM */
}

void ui_led_set_state(u32_t led, u8_t value)
{
#if !defined(CONFIG_UI_LED_USE_PWM)
	if (value) {
		current_led_state |= BIT(led - 1);
	} else {
		current_led_state &= ~BIT(led - 1);
	}
#endif
}

int ui_init(ui_callback_t cb)
{
	int err = 0;

#ifdef CONFIG_UI_LED_USE_PWM
	err = ui_leds_init();
	if (err) {
		LOG_ERR("Error when initializing PWM controlled LEDs");
		return err;
	}
#else
	err = dk_leds_init();
	if (err) {
		LOG_ERR("Could not initialize leds, err code: %d\n", err);
		return err;
	}

	err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
	if (err) {
		LOG_ERR("Could not set leds state, err code: %d\n", err);
		return err;
	}

	k_delayed_work_init(&leds_update_work, leds_update);
	k_delayed_work_submit(&leds_update_work, K_NO_WAIT);
#endif /* CONFIG_UI_LED_USE_PWM */

	if (cb) {
		callback  = cb;

		err = dk_buttons_init(button_handler);
		if (err) {
			LOG_ERR("Could not initialize buttons, err code: %d\n",
				err);
			return err;
		}
	}

#ifdef CONFIG_UI_BUZZER
	err = ui_buzzer_init();
	if (err) {
		LOG_ERR("Could not enable buzzer, err code: %d\n", err);
		return err;
	}
#endif /* CONFIG_UI_BUZZER */

#ifdef CONFIG_UI_NMOS
	err = ui_nmos_init();
	if (err) {
		LOG_ERR("Could not enable NMOS control, err code: %d\n", err);
		return err;
	}
#endif /* CONFIG_UI_NMOS */

	return err;
}

bool ui_button_is_active(u32_t button)
{
#if defined(CONFIG_DK_LIBRARY)
	return dk_get_buttons() & BIT((button - 1));
#else
	return false;
#endif
}
