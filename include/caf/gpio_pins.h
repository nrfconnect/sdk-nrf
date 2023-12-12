/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _GPIO_PINS_H_
#define _GPIO_PINS_H_

#include <stdbool.h>
#include <stdint.h>

enum pull_resistor {
	PIN_NO_PULL = -1,
	PIN_PULL_DEFAULT = 0,
	PIN_PULL_DOWN = 1,
	PIN_PULL_UP = 2
};

struct gpio_pin {
	uint8_t port;
	uint8_t pin;
	bool wakeup_blocked;
	enum pull_resistor pull_resistor;
};

#endif /* _GPIO_PINS_H_ */
