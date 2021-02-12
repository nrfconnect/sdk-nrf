/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <dk_buttons_and_leds.h>
#include <drivers/pwm.h>
#include <zboss_api.h>
#include <zb_led_button.h>


#define LED_PWM_PERIOD_US (USEC_PER_SEC / 50U)
#define FLAGS_OR_ZERO(node) \
	COND_CODE_1(DT_PHA_HAS_CELL(node, pwms, flags), \
		    (DT_PWMS_FLAGS(node)), (0))

#define PWM_DK_LED_LABEL(idx)   DT_PWMS_LABEL(DT_NODELABEL(pwm_led##idx))
#define PWM_DK_LED_CHANNEL(idx) DT_PWMS_CHANNEL(DT_NODELABEL(pwm_led##idx))
#define PWM_DK_LED_FLAGS(idx)   FLAGS_OR_ZERO(DT_NODELABEL(pwm_led##idx))


#ifdef CONFIG_ZIGBEE_USE_DIMMABLE_LED
static uint32_t pwm_channel;
static pwm_flags_t pwm_flags;
static const struct device *led_pwm_dev;
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
	char *driver_name = NULL;

	if (led_pwm_dev != NULL) {
		/* Driver is already initialized. */
		return ZB_FALSE;
	}

	switch (led_no) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm_led0), okay)
	case 0:
		driver_name = PWM_DK_LED_LABEL(0);
		pwm_channel = PWM_DK_LED_CHANNEL(0);
		pwm_flags = PWM_DK_LED_FLAGS(0);
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm_led1), okay)
	case 1:
		driver_name = PWM_DK_LED_LABEL(1);
		pwm_channel = PWM_DK_LED_CHANNEL(1);
		pwm_flags = PWM_DK_LED_FLAGS(1);
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm_led2), okay)
	case 2:
		driver_name = PWM_DK_LED_LABEL(2);
		pwm_channel = PWM_DK_LED_CHANNEL(2);
		pwm_flags = PWM_DK_LED_FLAGS(2);
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm_led3), okay)
	case 3:
		driver_name = PWM_DK_LED_LABEL(3);
		pwm_channel = PWM_DK_LED_CHANNEL(3);
		pwm_flags = PWM_DK_LED_FLAGS(3);
		break;
#endif
	default:
		return ZB_FALSE;
	}

	led_pwm_dev = device_get_binding(driver_name);
	if (!led_pwm_dev) {
		LOG_ERR("Cannot find %s!", driver_name);

		return ZB_FALSE;
	}

	return ZB_TRUE;
}

void zb_osif_led_on_set_level(zb_uint8_t level)
{
	uint32_t pulse = level * LED_PWM_PERIOD_US / 255U;

	if (!led_pwm_dev) {
		return;
	}

	if (pwm_pin_set_usec(led_pwm_dev, pwm_channel, LED_PWM_PERIOD_US,
			     pulse, pwm_flags)) {
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
	int err = dk_buttons_init(button_handler);

	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);

		return ZB_FALSE;
	}

	return ZB_TRUE;
}

int zb_osif_button_state(zb_uint8_t arg)
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
