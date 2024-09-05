/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(nrf_ps_server, CONFIG_NRF_PS_SERVER_LOG_LEVEL);

/*
 * This module intentionally do not use DK library.
 * On nRF54l15 default buttons overlap with UART pins configuration.
 */

#define SW0_NODE DT_ALIAS(sw3)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_ERR("Button causing k_oops pressed.\n");
	k_oops();
}

static int button_handler_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		LOG_DBG("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		LOG_DBG("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
			button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_DBG("Error %d: failed to configure interrupt on %s pin %d\n", ret,
			button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	LOG_DBG("Set up button at %s pin %d\n", button.port->name, button.pin);

	return 0;
}

SYS_INIT(button_handler_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
