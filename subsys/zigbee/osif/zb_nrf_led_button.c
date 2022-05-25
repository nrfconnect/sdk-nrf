/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/pwm.h>
#include <zboss_api.h>
#include <zb_led_button.h>


#define LED_PWM_PERIOD_US (USEC_PER_SEC / 100U)

#define LED_PWM_ENABLED(idx) DT_NODE_HAS_STATUS(DT_NODELABEL(pwm_led##idx), okay)
#define LED_PWM_DT_SPEC(idx) PWM_DT_SPEC_GET(DT_NODELABEL(pwm_led##idx))

#ifdef CONFIG_ZIGBEE_USE_DIMMABLE_LED
static const struct pwm_dt_spec *led_pwm;
#if LED_PWM_ENABLED(0)
static const struct pwm_dt_spec led_pwm0 = LED_PWM_DT_SPEC(0);
#endif
#if LED_PWM_ENABLED(1)
static const struct pwm_dt_spec led_pwm1 = LED_PWM_DT_SPEC(1);
#endif
#if LED_PWM_ENABLED(2)
static const struct pwm_dt_spec led_pwm2 = LED_PWM_DT_SPEC(2);
#endif
#if LED_PWM_ENABLED(3)
static const struct pwm_dt_spec led_pwm3 = LED_PWM_DT_SPEC(3);
#endif
#endif /* CONFIG_ZIGBEE_USE_DIMMABLE_LED */

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);


#ifdef CONFIG_ZIGBEE_USE_BUTTONS
static void button_update_state(zb_uint8_t button_id, zb_bool_t state)
{
	if (state) {
		zb_button_on_cb(button_id);
	} else {
		zb_button_off_cb(button_id);
	}
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	switch (has_changed) {
	case DK_BTN1_MSK:
		button_update_state(0, button_state & DK_BTN1_MSK);
		break;
	case DK_BTN2_MSK:
		button_update_state(1, button_state & DK_BTN2_MSK);
		break;
	case DK_BTN3_MSK:
		button_update_state(2, button_state & DK_BTN3_MSK);
		break;
	case DK_BTN4_MSK:
		button_update_state(3, button_state & DK_BTN4_MSK);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_ZIGBEE_USE_BUTTONS */


#ifdef CONFIG_ZIGBEE_USE_LEDS
void zb_osif_led_button_init(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

void zb_osif_led_on(zb_uint8_t led_no)
{
	switch (led_no) {
	case 0:
		dk_set_led(DK_LED1, 1);
		break;
	case 1:
		dk_set_led(DK_LED2, 1);
		break;
	case 2:
		dk_set_led(DK_LED3, 1);
		break;
	case 3:
		dk_set_led(DK_LED4, 1);
		break;
	default:
		break;
	}
}

void zb_osif_led_off(zb_uint8_t led_no)
{
	switch (led_no) {
	case 0:
		dk_set_led(DK_LED1, 0);
		break;
	case 1:
		dk_set_led(DK_LED2, 0);
		break;
	case 2:
		dk_set_led(DK_LED3, 0);
		break;
	case 3:
		dk_set_led(DK_LED4, 0);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_ZIGBEE_USE_LEDS */

#ifdef CONFIG_ZIGBEE_USE_DIMMABLE_LED
zb_bool_t zb_osif_led_level_init(zb_uint8_t led_no)
{
	if (led_pwm != NULL) {
		/* Driver is already initialized. */
		return ZB_FALSE;
	}

	switch (led_no) {
#if LED_PWM_ENABLED(0)
	case 0:
		led_pwm = &led_pwm0;
		break;
#endif
#if LED_PWM_ENABLED(1)
	case 1:
		led_pwm = &led_pwm1;
		break;
#endif
#if LED_PWM_ENABLED(2)
	case 2:
		led_pwm = &led_pwm2;
		break;
#endif
#if LED_PWM_ENABLED(3)
	case 3:
		led_pwm = &led_pwm3;
		break;
#endif
	default:
		return ZB_FALSE;
	}

	if (!device_is_ready(led_pwm->dev)) {
		LOG_ERR("Device %s is not ready!", led_pwm->dev->name);

		return ZB_FALSE;
	}

	return ZB_TRUE;
}

void zb_osif_led_on_set_level(zb_uint8_t level)
{
	uint32_t pulse = level * LED_PWM_PERIOD_US / 255U;

	if (pwm_set_dt(led_pwm, PWM_USEC(LED_PWM_PERIOD_US), PWM_USEC(pulse))) {
		LOG_ERR("Pwm led 4 set fails:\n");
	}
}
#endif /* CONFIG_ZIGBEE_USE_DIMMABLE_LED */

#ifdef CONFIG_ZIGBEE_USE_BUTTONS
void zb_osif_button_cb(zb_uint8_t arg)
{
	/* Intentionally left empty. */
}

zb_bool_t zb_setup_buttons_cb(zb_callback_t cb)
{
	static bool is_init;
	int err;

	if (is_init) {
		return ZB_TRUE;
	}
	err = dk_buttons_init(button_handler);

	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);

		return ZB_FALSE;
	}
	is_init = true;

	return ZB_TRUE;
}

zb_bool_t zb_osif_button_state(zb_uint8_t arg)
{
	uint32_t button_state, has_changed;
	uint32_t button_mask = 0;

	dk_read_buttons(&button_state, &has_changed);

	switch (arg) {
	case 0:
		button_mask = DK_BTN1_MSK;
		break;
	case 1:
		button_mask = DK_BTN2_MSK;
		break;
	case 2:
		button_mask = DK_BTN3_MSK;
		break;
	case 3:
		button_mask = DK_BTN4_MSK;
		break;
	default:
		break;
	}

	return (button_state & button_mask ? ZB_FALSE : ZB_TRUE);
}
#endif /* CONFIG_ZIGBEE_USE_BUTTONS */
