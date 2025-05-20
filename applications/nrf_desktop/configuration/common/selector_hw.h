/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SELECTOR_HW_H_
#define _SELECTOR_HW_H_

#include <caf/gpio_pins.h>

struct selector_config {
	uint8_t id;
	const struct gpio_pin *pins;
	uint8_t pins_size;
};

#endif /* _SELECTOR_HW_H_ */
