/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MAIN_H__
#define MAIN_H__

#include <zephyr/kernel.h>
#include <drivers/gpio/nrfe_gpio.h>

#if !defined(CONFIG_GPIO_NRFE_EGPIO_BACKEND_ICMSG) && !defined(CONFIG_GPIO_NRFE_EGPIO_BACKEND_MBOX)
#error "Define communication backend type"
#endif

void process_packet(nrfe_gpio_data_packet_t *packet);
int backend_init(void);

#endif /* MAIN_H__ */
