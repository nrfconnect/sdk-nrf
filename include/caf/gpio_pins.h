/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _GPIO_PINS_H_
#define _GPIO_PINS_H_

static const char * const port_map[] = {
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay),
			(DT_LABEL(DT_NODELABEL(gpio0))), (NULL)),
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay),
			(DT_LABEL(DT_NODELABEL(gpio1))), (NULL))
};

struct gpio_pin {
	uint8_t port;
	uint8_t pin;
};

#endif /* _GPIO_PINS_H_ */
