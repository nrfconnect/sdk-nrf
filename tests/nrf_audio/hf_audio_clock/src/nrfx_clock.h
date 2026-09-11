/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFX_CLOCK_H__
#define NRFX_CLOCK_H__

#include <stdbool.h>

/* Test-only declarations intentionally shadow nrfx hardware headers. */

typedef enum {
	NRF_CLOCK_DOMAIN_HFCLK,
} nrf_clock_domain_t;

typedef enum {
	NRF_CLOCK_HFCLK_DIV_1,
} nrf_clock_hfclk_div_t;

int nrfx_clock_divider_set(nrf_clock_domain_t domain, nrf_clock_hfclk_div_t div);

#endif
