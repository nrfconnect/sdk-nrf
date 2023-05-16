/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_driver.h>
#include <npmx_led.h>

#define LOG_MODULE_NAME pmic_led
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/**
 * @brief Set LEDs status based on lower 3 bits of status variable.
 *
 * @param[in] p_pm   Pointer to the instance of nPM device.
 * @param[in] status Lower 3 bits are used to set or reset LEDs.
 */
void leds_status_set(npmx_instance_t *p_pm, uint8_t status)
{
	npmx_led_state_set(npmx_led_get(p_pm, 0), (status & (1UL << 0UL)) > 0);
	npmx_led_state_set(npmx_led_get(p_pm, 1), (status & (1UL << 1UL)) > 0);
	npmx_led_state_set(npmx_led_get(p_pm, 2), (status & (1UL << 2UL)) > 0);
}

void main(void)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return;
	}

	LOG_INF("PMIC device ok");

	/* Get the pointer to npmx device. */
	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	/* Leds sequence to set. Only first 3 bits are used. */
	const uint8_t led_sequence[] = { 0b111, 0b011, 0b001, 0b010, 0b000 };
	const uint8_t led_sequence_count = sizeof(led_sequence);

	/* Set all LEDs to be controlled by the host. */
	npmx_led_mode_set(npmx_led_get(npmx_instance, 0), NPMX_LED_MODE_HOST);
	npmx_led_mode_set(npmx_led_get(npmx_instance, 1), NPMX_LED_MODE_HOST);
	npmx_led_mode_set(npmx_led_get(npmx_instance, 2), NPMX_LED_MODE_HOST);

	while (1) {
		for (uint8_t i = 0; i < led_sequence_count; i++) {
			leds_status_set(npmx_instance, led_sequence[i]);
			k_msleep(500);
		}
		LOG_INF("LEDs blinking");
	}
}
