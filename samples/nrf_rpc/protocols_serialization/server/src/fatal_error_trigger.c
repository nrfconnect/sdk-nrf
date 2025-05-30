/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_DECLARE(nrf_ps_server, CONFIG_NRF_PS_SERVER_LOG_LEVEL);

const struct gpio_dt_spec gpio_pin = GPIO_DT_SPEC_GET(DT_ALIAS(fatal_error_trigger), gpios);
static struct gpio_callback gpio_cb;

static void trigger_handler(const struct device *port, struct gpio_callback *cb,
			    gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	LOG_ERR("Fatal error button pressed - triggering k_oops");
	k_oops();
}

static int trigger_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&gpio_pin)) {
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&gpio_pin, GPIO_INPUT);
	if (rc) {
		return rc;
	}

	gpio_init_callback(&gpio_cb, trigger_handler, BIT(gpio_pin.pin));

	rc = gpio_add_callback_dt(&gpio_pin, &gpio_cb);
	if (rc) {
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&gpio_pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc) {
		return rc;
	}

	return 0;
}

SYS_INIT(trigger_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
