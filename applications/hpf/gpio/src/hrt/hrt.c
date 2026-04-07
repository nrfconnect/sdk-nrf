/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>

extern volatile uint16_t irq_arg;
extern volatile uint16_t irq_arg2;

void hrt_set_bits(void)
{
	uint16_t outs = nrf_vpr_csr_vio_out_get();

	nrf_vpr_csr_vio_out_set(outs | irq_arg);
}

void hrt_clear_bits(void)
{
	uint16_t outs = nrf_vpr_csr_vio_out_get();

	nrf_vpr_csr_vio_out_set(outs & ~irq_arg);
}

void hrt_toggle_bits(void)
{
	nrf_vpr_csr_vio_out_toggle_set(irq_arg);
}

void hrt_set_masked_bits(void)
{
	uint16_t outs = nrf_vpr_csr_vio_out_get();

	outs = (outs & ~irq_arg) | (irq_arg2 & irq_arg);
	nrf_vpr_csr_vio_out_set(outs);
}
