/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-npm6001-gpio.h>
#include <zephyr/dt-bindings/regulator/npm6001.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(thingy91x_board, CONFIG_BOARD_LOG_LEVEL);

static const struct device *wifi_regulator = DEVICE_DT_GET(DT_NODELABEL(reg_wifi));

static const struct gpio_dt_spec ldsw_rf_fe_sr_en = {
	.port = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(ldsw_rf_fe_sr_en))),
	.pin = 1,
	.dt_flags = GPIO_ACTIVE_LOW | GPIO_OPEN_DRAIN | GPIO_PULL_UP,
};

/* Enable the regulator and set the GPIO pin to enable the RF frontend */
int nrf_wifi_if_zep_start_board(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;

	ret = regulator_enable(wifi_regulator);
	if (ret) {
		LOG_ERR("Cannot turn on regulator %s (%d)", wifi_regulator->name, ret);
		return ret;
	}
	ret = regulator_set_mode(wifi_regulator, NPM6001_MODE_PWM);
	if (ret) {
		LOG_ERR("Cannot set mode for regulator %s (%d)", wifi_regulator->name, ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&ldsw_rf_fe_sr_en, 1);
	if (ret) {
		LOG_ERR("Cannot set GPIO %s (%d)", ldsw_rf_fe_sr_en.port->name, ret);
		return ret;
	}

	/* Wait for the load switch to settle on operating voltage. */
	k_sleep(K_USEC(300));

	return 0;
}

/* Disable the regulator and set the GPIO pin to disable the RF frontend */
int nrf_wifi_if_zep_stop_board(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret;

	ret = regulator_disable(wifi_regulator);
	if (ret) {
		LOG_ERR("Cannot turn off regulator %s (%d)", wifi_regulator->name, ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&ldsw_rf_fe_sr_en, 0);
	if (ret) {
		LOG_ERR("Cannot set GPIO %s (%d)", ldsw_rf_fe_sr_en.port->name, ret);
		return ret;
	}

	return 0;
}
