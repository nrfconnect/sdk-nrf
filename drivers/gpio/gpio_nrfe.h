/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GPIO_NRFE_H__
#define GPIO_NRFE_H__

#include <drivers/gpio/nrfe_gpio.h>
#include <zephyr/device.h>

#if !defined(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICMSG) &&    \
	!defined(CONFIG_GPIO_NRFE_EGPIO_BACKEND_MBOX) && \
	!defined(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICBMSG)
#error "Configure communication backend type"
#endif

int gpio_send(nrfe_gpio_data_packet_t *msg);
int gpio_nrfe_init(const struct device *port);

#endif /* GPIO_NRFE_H__ */
