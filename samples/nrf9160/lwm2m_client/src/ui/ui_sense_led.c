/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m_client, CONFIG_APP_LOG_LEVEL);

static const struct gpio_dt_spec red_led =
	GPIO_DT_SPEC_GET(DT_NODELABEL(sense_red_led), gpios);
static const struct gpio_dt_spec green_led =
	GPIO_DT_SPEC_GET(DT_NODELABEL(sense_green_led), gpios);
static const struct gpio_dt_spec blue_led =
	GPIO_DT_SPEC_GET(DT_NODELABEL(sense_blue_led), gpios);

int ui_sense_led_on_off(bool new_state)
{
	int ret;

	ret = gpio_pin_set_dt(&red_led, (int)new_state);
	if (ret) {
		LOG_ERR("Set red pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_set_dt(&green_led, (int)new_state);
	if (ret) {
		LOG_ERR("Set green pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_set_dt(&blue_led, (int)new_state);
	if (ret) {
		LOG_ERR("Set blue pin failed (%d)", ret);
		return ret;
	}
	return 0;
}

int ui_sense_led_init(void)
{
	int ret;

	if (!device_is_ready(red_led.port) ||
	    !device_is_ready(green_led.port) ||
	    !device_is_ready(blue_led.port)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Configure red pin failed (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Configure green pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Configure blue pin failed (%d)", ret);
		return ret;
	}

	return 0;
}
