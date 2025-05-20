/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _GPIO_PINS_H_
#define _GPIO_PINS_H_

#include <stdbool.h>
#include <stdint.h>

struct gpio_pin {
	uint8_t port;
	uint8_t pin;
	bool wakeup_blocked;
};

#endif /* _GPIO_PINS_H_ */
