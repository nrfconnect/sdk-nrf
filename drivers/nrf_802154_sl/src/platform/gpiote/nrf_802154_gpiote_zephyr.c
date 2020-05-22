/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 *   This file contains implementation of the nRF 802.15.4 GPIOTE abstraction.
 *
 * This implementation is to be used with Zephyr RTOS.
 *
 */

#include <platform/gpiote/nrf_802154_gpiote.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <device.h>
#include <toolchain.h>
#include <drivers/gpio.h>

#include <hal/nrf_gpio.h>
#include <nrf_802154_sl_coex.h>
#include <nrf_802154_sl_utils.h>

static struct device *dev;
static struct gpio_callback grant_cb;
static uint32_t pin_number = COEX_GPIO_PIN_INVALID;

static void gpiote_irq_handler(struct device *gpiob, struct gpio_callback *cb,
			       u32_t pins)
{
	ARG_UNUSED(gpiob);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	nrf_802154_wifi_coex_gpiote_irqhandler();
}

void nrf_802154_gpiote_init(void)
{
	switch (nrf_802154_wifi_coex_interface_type_id_get()) {
	case NRF_802154_WIFI_COEX_IF_NONE:
		return;

	case NRF_802154_WIFI_COEX_IF_3WIRE: {
		nrf_802154_wifi_coex_3wire_if_config_t cfg;

		nrf_802154_wifi_coex_cfg_3wire_get(&cfg);

		pin_number = cfg.grant_cfg.gpio_pin;
		assert(pin_number != COEX_GPIO_PIN_INVALID);

		bool use_port_1 = (pin_number > P0_PIN_NUM);

		/* Convert to the Zephyr primitive */
		pin_number = use_port_1 ? pin_number - P0_PIN_NUM : pin_number;

		uint32_t pull_up_down = cfg.grant_cfg.active_high ?
						GPIO_PULL_UP :
						GPIO_PULL_DOWN;

		dev = device_get_binding(use_port_1 ? "GPIO_1" : "GPIO_0");
		assert(dev != NULL);

		gpio_pin_configure(dev, pin_number,
				   GPIO_INPUT | GPIO_INT_ENABLE | GPIO_INT_EDGE |
					   GPIO_INT_EDGE_BOTH | pull_up_down);

		gpio_init_callback(&grant_cb, gpiote_irq_handler, BIT(pin_number));
		gpio_add_callback(dev, &grant_cb);
		gpio_enable_callback(dev, pin_number);
		break;
	}

	default:
		assert(false);
	}
}

void nrf_802154_gpiote_deinit(void)
{
	gpio_disable_callback(dev, pin_number);
	gpio_remove_callback(dev, &grant_cb);
	pin_number = COEX_GPIO_PIN_INVALID;
}
