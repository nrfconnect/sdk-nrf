/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/random/random.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_uarte.h>
#include <nrfx_gpiote.h>

#define DT_DRV_COMPAT nordic_nrf_sw_lpuart

struct test_data {
	struct counter_alarm_cfg alarm_cfg;
	int32_t tx_pin;
};

void floating_pins_start(int32_t tx_pin);

void floating_pins_stop(int32_t tx_pin);
