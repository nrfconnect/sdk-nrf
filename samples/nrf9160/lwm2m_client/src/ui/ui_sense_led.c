/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ui_sense_led, CONFIG_UI_LOG_LEVEL);

#define SENSE_GPIO_NODE(led_label) DT_NODELABEL(led_label)
#define SENSE_GPIO_PIN(led_label) DT_GPIO_PIN(SENSE_GPIO_NODE(led_label), gpios)
#define SENSE_GPIO_FLAGS DT_GPIO_FLAGS(SENSE_GPIO_NODE(sense_red_led), gpios)
#define SENSE_GPIO_DEV_LABEL DT_GPIO_LABEL(SENSE_GPIO_NODE(sense_red_led), gpios)

static const struct device *sense_led_gpio_dev;

int ui_sense_led_on_off(bool new_state)
{
	int ret;

	ret = gpio_pin_set(sense_led_gpio_dev, SENSE_GPIO_PIN(sense_red_led), new_state);
	if (ret) {
		LOG_ERR("Set red pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_set(sense_led_gpio_dev, SENSE_GPIO_PIN(sense_green_led), new_state);
	if (ret) {
		LOG_ERR("Set green pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_set(sense_led_gpio_dev, SENSE_GPIO_PIN(sense_blue_led), new_state);
	if (ret) {
		LOG_ERR("Set blue pin failed (%d)", ret);
		return ret;
	}
	return 0;
}

int ui_sense_led_init(void)
{
	int ret;

	sense_led_gpio_dev = device_get_binding(SENSE_GPIO_DEV_LABEL);
	if (!sense_led_gpio_dev) {
		LOG_ERR("Could not bind to GPIO device (%d)", -ENODEV);
		return -ENODEV;
	}

	ret = gpio_pin_configure(sense_led_gpio_dev, SENSE_GPIO_PIN(sense_red_led),
				 SENSE_GPIO_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Configure red pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_configure(sense_led_gpio_dev, SENSE_GPIO_PIN(sense_green_led),
				 SENSE_GPIO_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Configure green pin failed (%d)", ret);
		return ret;
	}
	ret = gpio_pin_configure(sense_led_gpio_dev, SENSE_GPIO_PIN(sense_blue_led),
				 SENSE_GPIO_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Configure blue pin failed (%d)", ret);
		return ret;
	}

	return 0;
}
