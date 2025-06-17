/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_utils.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/__assert.h>
#include <hal/nrf_gpio.h>
#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif
#if defined(NRF54L_SERIES)
#include <hal/nrf_gpiote.h>
#include <nrfx_ppib.h>
#include <helpers/nrfx_gppi.h>
#endif

int mpsl_fem_utils_ppi_channel_alloc(uint8_t *ppi_channels, size_t size)
{
	nrfx_err_t err = NRFX_ERROR_NOT_SUPPORTED;
#ifdef DPPI_PRESENT
#if defined(NRF53_SERIES)
	nrfx_dppi_t dppi = NRFX_DPPI_INSTANCE(0);
#elif defined(NRF54L_SERIES)
	nrfx_dppi_t dppi = NRFX_DPPI_INSTANCE(10);
#else
#error Unsupported SoC series
#endif
#endif

	for (int i = 0; i < size; i++) {
		IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
			(err = nrfx_ppi_channel_alloc(&ppi_channels[i]);));
		IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
			(err = nrfx_dppi_channel_alloc(&dppi, &ppi_channels[i]);));
		if (err != NRFX_SUCCESS) {
			return -ENOMEM;
		}
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

#if !defined(_MPSL_FEM_CONFIG_API_NEXT)
int mpsl_fem_utils_gpiote_pin_init(mpsl_fem_gpiote_pin_config_t *gpiote_pin)
{
#if defined(NRF54L_SERIES)
	nrfx_err_t err;
	uint8_t ppib_ch = 0;
	uint8_t gppi_ch = 0;
	nrfx_ppib_interconnect_t ppib11_21 = NRFX_PPIB_INTERCONNECT_INSTANCE(11, 21);

	err = nrfx_ppib_channel_alloc(&ppib11_21, &ppib_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	gpiote_pin->ppib_channels[0] = ppib_ch;

	err = nrfx_gppi_channel_alloc(&gppi_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	nrfx_gppi_channel_endpoints_setup(gppi_ch,
		nrfx_ppib_receive_event_address_get(&ppib11_21.right, ppib_ch),
		nrf_gpiote_task_address_get(gpiote_pin->p_gpiote,
			nrf_gpiote_clr_task_get(gpiote_pin->gpiote_ch_id)));

	nrfx_gppi_channels_enable(1U << gppi_ch);

	err = nrfx_ppib_channel_alloc(&ppib11_21, &ppib_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	gpiote_pin->ppib_channels[1] = ppib_ch;

	err = nrfx_gppi_channel_alloc(&gppi_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	nrfx_gppi_channel_endpoints_setup(gppi_ch,
			nrfx_ppib_receive_event_address_get(&ppib11_21.right, ppib_ch),
			nrf_gpiote_task_address_get(gpiote_pin->p_gpiote,
				nrf_gpiote_set_task_get(gpiote_pin->gpiote_ch_id)));

	nrfx_gppi_channels_enable(1U << gppi_ch);
#endif
	return 0;
}
#endif /* !defined(_MPSL_FEM_CONFIG_API_NEXT) */
