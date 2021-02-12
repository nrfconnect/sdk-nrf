/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrfx_clock.h>
#include <mpsl.h>
#include <mpsl_clock.h>

static nrfx_clock_event_handler_t event_handler;

static void mpsl_hfclk_callback(void)
{
	event_handler(NRFX_CLOCK_EVT_HFCLK_STARTED);
}

void nrfx_clock_start(nrf_clock_domain_t domain)
{
	switch (domain) {
	case NRF_CLOCK_DOMAIN_HFCLK:
		mpsl_clock_hfclk_request(mpsl_hfclk_callback);
		break;
	case NRF_CLOCK_DOMAIN_LFCLK:
		event_handler(NRFX_CLOCK_EVT_LFCLK_STARTED);
		break;
	default:
		__ASSERT(0, "Not supported");
	}
}

void nrfx_clock_stop(nrf_clock_domain_t domain)
{
	switch (domain) {
	case NRF_CLOCK_DOMAIN_HFCLK:
		mpsl_clock_hfclk_release();
		break;
	case NRF_CLOCK_DOMAIN_LFCLK:
		/* empty */
		break;
	default:
		__ASSERT(0, "Not supported");
	}
}

void nrfx_clock_enable(void)
{

}

nrfx_err_t nrfx_clock_init(nrfx_clock_event_handler_t handler)
{
	event_handler = handler;

	return NRFX_SUCCESS;
}


void nrfx_clock_irq_handler(void)
{
	MPSL_IRQ_CLOCK_Handler();
}
