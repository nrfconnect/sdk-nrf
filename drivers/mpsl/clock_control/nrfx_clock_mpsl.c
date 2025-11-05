/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <nrfx_clock.h>

#include <mpsl.h>
#include <mpsl_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrfx_clock_mpsl, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static nrfx_clock_event_handler_t event_handler;

static void mpsl_hfclk_src_callback(mpsl_clock_evt_type_t evt_type)
{
	switch (evt_type) {
#if NRF_CLOCK_HAS_XO_TUNE
	/* nRF clock driver doesn't do anything with NRFX_CLOCK_EVT_XO_TUNE_ERROR and
	 * NRFX_CLOCK_EVT_XO_TUNE_FAILED events. There is no need to report them.
	 */
	case MPSL_CLOCK_EVT_XO_TUNED:
		event_handler(NRFX_CLOCK_EVT_XO_TUNED);
		break;
#endif /* NRF_CLOCK_HAS_XO_TUNE */
	case MPSL_CLOCK_EVT_HFCLK_STARTED:
		event_handler(NRFX_CLOCK_EVT_HFCLK_STARTED);
		break;
#if NRF_CLOCK_HAS_HFCLK24M
	case MPSL_CLOCK_EVT_HFCLK24M_STARTED:
		event_handler(NRFX_CLOCK_EVT_HFCLK24M_STARTED);
		break;
#endif /* NRF_CLOCK_HAS_HFCLK24M */
	default:
		/* We do not send notification about any other clock event to higher level driver */
		LOG_WRN("Unsupported HFCLK event: %d", evt_type);
	}
}

void nrfx_clock_lfclk_start(void)
{
	nrfx_clock_start(NRF_CLOCK_DOMAIN_LFCLK);
}

void nrfx_clock_lfclk_stop(void)
{
	nrfx_clock_stop(NRF_CLOCK_DOMAIN_LFCLK);
}

void nrfx_clock_hfclk_start(void)
{
	nrfx_clock_start(NRF_CLOCK_DOMAIN_HFCLK);
}

void nrfx_clock_hfclk_stop(void)
{
	nrfx_clock_stop(NRF_CLOCK_DOMAIN_HFCLK);
}

void nrfx_clock_start(nrf_clock_domain_t domain)
{
	switch (domain) {
	case NRF_CLOCK_DOMAIN_HFCLK:
		mpsl_clock_hfclk_src_request(MPSL_CLOCK_HF_SRC_XO, mpsl_hfclk_src_callback);
		break;
#if NRF_CLOCK_HAS_HFCLK24M
	case NRF_CLOCK_DOMAIN_HFCLK24M:
		mpsl_clock_hfclk_src_request(MPSL_CLOCK_HF_SRC_HFCLK24M, mpsl_hfclk_src_callback);
		break;
#endif /* NRF_CLOCK_HAS_HFCLK24M */
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
		mpsl_clock_hfclk_src_release(MPSL_CLOCK_HF_SRC_XO);
		break;
#if NRF_CLOCK_HAS_HFCLK24M
	case NRF_CLOCK_DOMAIN_HFCLK24M:
		mpsl_clock_hfclk_src_release(MPSL_CLOCK_HF_SRC_HFCLK24M);
		break;
#endif /* NRF_CLOCK_HAS_HFCLK24M */
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
