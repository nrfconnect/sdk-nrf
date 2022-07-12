/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <logging/log.h>
#include <nrfx.h>
#include <nrf52_dbg_pins.h>

struct gpio_pin {
	const char * const port;
	const uint8_t number;
};

static const struct gpio_pin dbg_pins[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	{DT_GPIO_LABEL(DT_ALIAS(sw0), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw0), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
	{DT_GPIO_LABEL(DT_ALIAS(sw1), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw1), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
	{DT_GPIO_LABEL(DT_ALIAS(sw2), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw2), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
	{DT_GPIO_LABEL(DT_ALIAS(sw3), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw3), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	{DT_GPIO_LABEL(DT_ALIAS(led0), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led0), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
	{DT_GPIO_LABEL(DT_ALIAS(led1), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led1), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
	{DT_GPIO_LABEL(DT_ALIAS(led2), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led2), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
	{DT_GPIO_LABEL(DT_ALIAS(led3), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led3), gpios)},
#endif
};

static const struct device *dbg_pin_devs[ARRAY_SIZE(dbg_pins)];
static bool is_initialized = false;

int nrf52_dbg_pins_init(void)
{
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(dbg_pins); i++) {
		dbg_pin_devs[i] = device_get_binding(dbg_pins[i].port);
		if (!dbg_pin_devs[i]) {
			return -ENODEV;
		}

		err = gpio_pin_configure(dbg_pin_devs[i], dbg_pins[i].number,
					 GPIO_OUTPUT);
		if (err) {
			return err;
		}
	}
	is_initialized = true;
	return nrf52_dbg_pin_port_set(0);
}

int nrf52_dbg_pin_port_set(uint8_t mask)
{
	if (!is_initialized) {
		nrf52_dbg_pins_init();
	}

	for (size_t i = 0; i < ARRAY_SIZE(dbg_pins); i++) {
		bool onoff = (BIT(i) & mask) ? true : false;

		int err = gpio_pin_set_raw(dbg_pin_devs[i],
						dbg_pins[i].number, (uint32_t)onoff);
		if (err) {
			return err;
		}
	}

	return 0;
}

int nrf52_dbg_pin_set(uint8_t dgb_pin_idx, bool onoff)
{
	if (!is_initialized) {
		nrf52_dbg_pins_init();
	}

	if (dgb_pin_idx > (ARRAY_SIZE(dbg_pins) - 1)) {
		return -EINVAL;
	}
	return gpio_pin_set_raw(dbg_pin_devs[dgb_pin_idx],
				dbg_pins[dgb_pin_idx].number, (uint32_t)onoff);
}

int nrf52_dbg_pin_on(uint8_t dgb_pin_idx)
{
	return nrf52_dbg_pin_set(dgb_pin_idx, true);
}

int nrf52_dbg_pin_off(uint8_t dgb_pin_idx)
{
	return nrf52_dbg_pin_set(dgb_pin_idx, false);
}

int nrf52_dbg_pin_spike(uint8_t dgb_pin_idx)
{
	int err = 0;
	err |= nrf52_dbg_pin_set(dgb_pin_idx, false);
	err |= nrf52_dbg_pin_set(dgb_pin_idx, true);
	err |= nrf52_dbg_pin_set(dgb_pin_idx, false);
	return err;
}
