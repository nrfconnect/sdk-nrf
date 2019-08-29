/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SELECTOR_HW_H_
#define _SELECTOR_HW_H_

#include "gpio_pins.h"

struct selector_config {
	u8_t id;
	const struct gpio_pin *pins;
	u8_t pins_size;
};

#endif /* _SELECTOR_HW_H_ */
