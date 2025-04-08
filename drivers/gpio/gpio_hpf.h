/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GPIO_HPF_H__
#define GPIO_HPF_H__

#include <drivers/gpio/hpf_gpio.h>
#include <zephyr/device.h>

#if !defined(CONFIG_GPIO_HPF_GPIO_BACKEND_ICMSG) &&    \
	!defined(CONFIG_GPIO_HPF_GPIO_BACKEND_MBOX) && \
	!defined(CONFIG_GPIO_HPF_GPIO_BACKEND_ICBMSG)
#error "Configure communication backend type"
#endif

int gpio_send(hpf_gpio_data_packet_t *msg);
int gpio_hpf_init(const struct device *port);

#endif /* GPIO_HPF_H__ */
