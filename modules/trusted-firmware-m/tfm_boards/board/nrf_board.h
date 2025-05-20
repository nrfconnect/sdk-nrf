/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_BOARD_H__
#define NRF_BOARD_H__

#include <zephyr/autoconf.h>
#include <zephyr/devicetree.h>

#include <hal/nrf_gpio.h>

#define BUTTON1_PIN	     (DT_GPIO_PIN(DT_NODELABEL(button0), gpios))
#define BUTTON1_ACTIVE_LEVEL (0UL)
#define BUTTON1_PULL	     (NRF_GPIO_PIN_PULLUP)
#define LED1_PIN	     (DT_GPIO_PIN(DT_NODELABEL(led0), gpios))
#define LED1_ACTIVE_LEVEL    (0UL)

#endif /* NRF_BOARD_H__ */
