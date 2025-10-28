/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_utils.h>
#include <hal/nrf_gpio.h>
#include <helpers/nrfx_gppi.h>

int mpsl_fem_utils_ppi_channel_alloc(uint8_t *ppi_channels, size_t size)
{
	int ch;
#if defined(NRF54L_SERIES)
	uint32_t domain_id = nrfx_gppi_domain_id_get((uint32_t)NRF_DPPIC10);
#elif defined(NRF53_SERIES) || defined(PPI_PRESENT)
	uint32_t domain_id = 0;
#else
#error Unsupported SoC series
#endif

	for (int i = 0; i < size; i++) {
		ch = nrfx_gppi_channel_alloc(domain_id, NULL);
		if (ch < 0) {
			return ch;
		}
		ppi_channels[i] = (uint8_t)ch;
	}

	return 0;
}

void mpsl_fem_extended_pin_to_mpsl_fem_pin(uint32_t pin_num, mpsl_fem_pin_t *p_fem_pin)
{
	// pin_num is saved, because nrf_gpio_pin_port_number_extract overwrites it and
	// its original value is needed for nrf_gpio_pin_port_decode
	uint32_t pin_num_copy = pin_num;

	p_fem_pin->port_no  = nrf_gpio_pin_port_number_extract(&pin_num_copy);
	p_fem_pin->p_port   = nrf_gpio_pin_port_decode(&pin_num);

	p_fem_pin->port_pin = pin_num;
}
