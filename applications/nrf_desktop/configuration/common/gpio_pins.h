/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef _GPIO_PINS_H_
#define _GPIO_PINS_H_


static const char * const port_map[] = {
#ifdef CONFIG_GPIO_NRF_P0
		DT_GPIO_P0_DEV_NAME,
#else
		NULL,
#endif /* CONFIG_GPIO_NRF_P0 */
#ifdef CONFIG_GPIO_NRF_P1
		DT_GPIO_P1_DEV_NAME,
#else
		NULL,
#endif /* CONFIG_GPIO_NRF_P1 */
};

struct gpio_pin {
	u8_t port;
	u8_t pin;
};

#endif /* _GPIO_PINS_H_ */
