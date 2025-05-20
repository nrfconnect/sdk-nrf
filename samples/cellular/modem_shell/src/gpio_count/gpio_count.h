/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_GPIO_COUNT_H
#define MOSH_GPIO_COUNT_H

#include <zephyr/types.h>

int gpio_count_enable(uint8_t pin);
int gpio_count_disable(void);
uint32_t gpio_count_get(void);
void gpio_count_reset(void);

#endif /* MOSH_GPIO_COUNT_H */
