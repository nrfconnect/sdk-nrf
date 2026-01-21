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
	uint32_t req_rdy_pin;
	uint32_t req_rdy_cnf;
};

/** @brief Starting pin floating.
 *
 *
 * @param use_req_pin Float REQ pin if true. Float RDY pin if false.
 * @param tx_pin Additionally float TX pin, valid only if @p use_req_pin is true.
 */
void floating_pins_start(bool use_req_pin, int32_t tx_pin);

/** @brief Stop pin floating. */
void floating_pins_stop(bool use_req_pin, int32_t tx_pin);
