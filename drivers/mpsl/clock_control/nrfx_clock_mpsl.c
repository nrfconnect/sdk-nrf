/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <nrfx_clock.h>

#if !defined(CONFIG_CLOCK_CONTROL_NRF)
#include <nrfx_clock_lfclk.h>
#if NRF_CLOCK_HAS_HFCLK
#include <nrfx_clock_hfclk.h>
#endif
#if NRF_CLOCK_HAS_XO
#include <nrfx_clock_xo.h>
#endif
#if NRF_CLOCK_HAS_HFCLK24M
#include <nrfx_clock_xo24m.h>
#endif

#include "clock_control_nrf_common.h"
#endif /* !defined(CONFIG_CLOCK_CONTROL_NRF) */

#include <mpsl.h>
#include <mpsl_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrfx_clock_mpsl, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
static nrfx_clock_event_handler_t m_event_handler;
#else
static nrfx_clock_lfclk_event_handler_t m_lfclk_event_handler;
#if NRF_CLOCK_HAS_HFCLK
static nrfx_clock_hfclk_event_handler_t m_hfclk_event_handler;
#endif
#if NRF_CLOCK_HAS_XO
static nrfx_clock_xo_event_handler_t m_xo_event_handler;
#endif
#if NRF_CLOCK_HAS_HFCLK24M
static nrfx_clock_xo24m_event_handler_t m_xo24m_event_handler;
#endif
#endif /* CONFIG_CLOCK_CONTROL_NRF */

static void mpsl_hfclk_src_callback(mpsl_clock_evt_type_t evt_type)
{
	switch (evt_type) {
#if NRF_CLOCK_HAS_XO_TUNE
	/* nRF clock driver doesn't do anything with NRFX_CLOCK_EVT_XO_TUNE_ERROR and
	 * NRFX_CLOCK_EVT_XO_TUNE_FAILED events. There is no need to report them.
	 */
	case MPSL_CLOCK_EVT_XO_TUNED:
#if defined(CONFIG_CLOCK_CONTROL_NRF)
		m_event_handler(NRFX_CLOCK_EVT_XO_TUNED);
#else
		if (m_xo_event_handler != NULL) {
			m_xo_event_handler(NRFX_CLOCK_XO_EVT_XO_TUNED);
		}
#endif /* CONFIG_CLOCK_CONTROL_NRF */
		break;
#endif /* NRF_CLOCK_HAS_XO_TUNE */
	case MPSL_CLOCK_EVT_HFCLK_STARTED:
#if defined(CONFIG_CLOCK_CONTROL_NRF)
		m_event_handler(NRFX_CLOCK_EVT_HFCLK_STARTED);
#else
#if NRF_CLOCK_HAS_XO
		if (m_xo_event_handler != NULL) {
			m_xo_event_handler(NRFX_CLOCK_XO_EVT_HFCLK_STARTED);
		}
#elif NRF_CLOCK_HAS_HFCLK
		if (m_hfclk_event_handler != NULL) {
			m_hfclk_event_handler();
		}
#endif /* NRF_CLOCK_HAS_XO */
#endif /* CONFIG_CLOCK_CONTROL_NRF */
		break;
#if NRF_CLOCK_HAS_HFCLK24M
	case MPSL_CLOCK_EVT_HFCLK24M_STARTED:
#if defined(CONFIG_CLOCK_CONTROL_NRF)
		m_event_handler(NRFX_CLOCK_EVT_HFCLK24M_STARTED);
#else
		if (m_xo24m_event_handler != NULL) {
			m_xo24m_event_handler();
		}
#endif /* CONFIG_CLOCK_CONTROL_NRF */
		break;
#endif /* NRF_CLOCK_HAS_HFCLK24M */
	default:
		/* We do not send notification about any other clock event to higher level driver */
		LOG_WRN("Unsupported HFCLK event: %d", evt_type);
		break;
	}
}

/*
 * LFCLK initialization flow:
 * 1. Zephyr lfclk driver init (PRE_KERNEL): nrfx_clock_lfclk_init() stores the event
 *    handler; onoff manager is created with LF off. No HW access.
 * 2. mpsl_init(): MPSL starts LFCLK in HW (clock_ctrl_init), optionally waits for
 *    LFSTAT, and handles LFCLKSTARTED in MPSL_IRQ_CLOCK_Handler().
 * 3. Later Zephyr LF request (onoff / z_nrf_clock_control_lf_on): lfclk_start() calls
 *    nrfx_clock_lfclk_start() below.
 *
 * Short turnaround: nrfx_clock_lfclk_start() does not start HW, wait for an IRQ, or
 * poll LFSTAT. LF is started by mpsl_init() (step 2). We only synchronously deliver
 * LFCLK_STARTED so the Zephyr onoff manager can leave STARTING (common_clkstarted_handle).
 *
 * Do not block here waiting for LF: this path can run from another driver's init
 * before mpsl_init(), while LF HW is still off. Blocking would stall that init (and
 * can deadlock if mpsl_init depends on it). HW readiness is established by mpsl_init
 * and, when needed, by lfclk_spinwait() in the caller (e.g. z_nrf_clock_control_lf_on).
 * Ensure mpsl_init runs before LF consumers (MPSL uses PRE_KERNEL_1 for CLOCK_CONTROL_MPSL)
 * or use CONFIG_SYSTEM_CLOCK_NO_WAIT to skip lfclk_spinwait().
 *
 * No Zephyr calibration handling: CLOCK_CONTROL_NRF_FORCE_ALT disables the Zephyr
 * driver calibration module. LFCLKSTARTED and CAL_DONE from HW are consumed in
 * MPSL_IRQ_CLOCK_Handler() (clock_ctrl_lfclk_calibration_init and ongoing RC cal).
 * They are not forwarded to the lfclk event handler; nrfx_clock_lfclk_irq_handler()
 * is intentionally empty.
 */
void nrfx_clock_lfclk_start(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	m_event_handler(NRFX_CLOCK_EVT_LFCLK_STARTED);
#else
	if (m_lfclk_event_handler != NULL) {
		m_lfclk_event_handler(NRFX_CLOCK_LFCLK_EVT_LFCLK_STARTED);
	}
#endif /* CONFIG_CLOCK_CONTROL_NRF */
}

void nrfx_clock_lfclk_stop(void)
{
	/* HW stop is owned by MPSL; Zephyr onoff stop does not stop LFCLK. */
}

#if NRF_CLOCK_HAS_HFCLK
void nrfx_clock_hfclk_start(void)
{
	int err = mpsl_clock_hfclk_src_request(MPSL_CLOCK_HF_SRC_XO, mpsl_hfclk_src_callback);

	if (err < 0) {
		__ASSERT(0, "Failed to request MPSL_CLOCK_HF_SRC_XO source: %d", err);
	}
}

void nrfx_clock_hfclk_stop(void)
{
	int err = mpsl_clock_hfclk_src_release(MPSL_CLOCK_HF_SRC_XO);

	if (err < 0) {
		__ASSERT(0, "Failed to release MPSL_CLOCK_HF_SRC_XO source: %d", err);
	}
}
#endif /* NRF_CLOCK_HAS_HFCLK */

#if NRF_CLOCK_HAS_XO
void nrfx_clock_xo_start(void)
{
	int err = mpsl_clock_hfclk_src_request(MPSL_CLOCK_HF_SRC_XO, mpsl_hfclk_src_callback);

	if (err < 0) {
		__ASSERT(0, "Failed to request MPSL_CLOCK_HF_SRC_XO source: %d", err);
	}
}

void nrfx_clock_xo_stop(void)
{
	int err = mpsl_clock_hfclk_src_release(MPSL_CLOCK_HF_SRC_XO);

	if (err < 0) {
		__ASSERT(0, "Failed to release MPSL_CLOCK_HF_SRC_XO source: %d", err);
	}
}
#endif /* NRF_CLOCK_HAS_XO */

#if NRF_CLOCK_HAS_HFCLK24M
void nrfx_clock_xo24m_start(void)
{
	int err = mpsl_clock_hfclk_src_request(MPSL_CLOCK_HF_SRC_HFCLK24M, mpsl_hfclk_src_callback);

	if (err < 0) {
		__ASSERT(0, "Failed to request MPSL_CLOCK_HF_SRC_HFCLK24M source: %d", err);
	}
}

void nrfx_clock_xo24m_stop(void)
{
	int err = mpsl_clock_hfclk_src_release(MPSL_CLOCK_HF_SRC_HFCLK24M);

	if (err < 0) {
		__ASSERT(0, "Failed to release MPSL_CLOCK_HF_SRC_HFCLK24M source: %d", err);
	}
}
#endif /* NRF_CLOCK_HAS_HFCLK24M */

void nrfx_clock_start(nrf_clock_domain_t domain)
{
	switch (domain) {
#if NRF_CLOCK_HAS_LFCLK
	case NRF_CLOCK_DOMAIN_LFCLK:
		nrfx_clock_lfclk_start();
		return;
#endif /* NRF_CLOCK_HAS_LFCLK */
	case NRF_CLOCK_DOMAIN_HFCLK:
#if NRF_CLOCK_HAS_XO
		nrfx_clock_xo_start();
#elif NRF_CLOCK_HAS_HFCLK
		nrfx_clock_hfclk_start();
#endif /* NRF_CLOCK_HAS_XO */
		return;
#if NRF_CLOCK_HAS_HFCLK24M
	case NRF_CLOCK_DOMAIN_HFCLK24M:
		nrfx_clock_xo24m_start();
		return;
#endif /* NRF_CLOCK_HAS_HFCLK24M */
	default:
		__ASSERT(0, "Not supported");
		return;
	}
}

void nrfx_clock_stop(nrf_clock_domain_t domain)
{
	switch (domain) {
#if NRF_CLOCK_HAS_LFCLK
	case NRF_CLOCK_DOMAIN_LFCLK:
		nrfx_clock_lfclk_stop();
		return;
#endif /* NRF_CLOCK_HAS_LFCLK */
	case NRF_CLOCK_DOMAIN_HFCLK:
#if NRF_CLOCK_HAS_XO
		nrfx_clock_xo_stop();
#elif NRF_CLOCK_HAS_HFCLK
		nrfx_clock_hfclk_stop();
#endif /* NRF_CLOCK_HAS_XO */
		return;
#if NRF_CLOCK_HAS_HFCLK24M
	case NRF_CLOCK_DOMAIN_HFCLK24M:
		nrfx_clock_xo24m_stop();
		return;
#endif /* NRF_CLOCK_HAS_HFCLK24M */
	default:
		__ASSERT(0, "Not supported");
		return;
	}
}

void nrfx_clock_enable(void)
{
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)
int nrfx_clock_init(nrfx_clock_event_handler_t handler)
{
	m_event_handler = handler;

	return 0;
}
#endif /* CONFIG_CLOCK_CONTROL_NRF */

/*
 * Name and global linkage are required by nrfx_power.c: nrfx_power_clock_irq_handler()
 * invokes this by the monolithic clock control driver (CONFIG_CLOCK_CONTROL_NRF).
 *
 * In the split clock control driver, there is no direct call site; the handler is invoked once
 * per CLOCK IRQ via CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE below.
 */
void nrfx_clock_irq_handler(void)
{
	MPSL_IRQ_CLOCK_Handler();
}

#if !defined(CONFIG_CLOCK_CONTROL_NRF)
CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_mpsl, &nrfx_clock_irq_handler);

int nrfx_clock_lfclk_init(nrfx_clock_lfclk_event_handler_t lfclk_event_handler)
{
	m_lfclk_event_handler = lfclk_event_handler;

	return 0;
}

#if NRF_CLOCK_HAS_HFCLK
int nrfx_clock_hfclk_init(nrfx_clock_hfclk_event_handler_t hfclk_event_handler)
{
	m_hfclk_event_handler = hfclk_event_handler;

	return 0;
}
#endif /* NRF_CLOCK_HAS_HFCLK */

#if NRF_CLOCK_HAS_XO
int nrfx_clock_xo_init(nrfx_clock_xo_event_handler_t xo_event_handler)
{
	m_xo_event_handler = xo_event_handler;

	return 0;
}
#endif /* NRF_CLOCK_HAS_XO */

#if NRF_CLOCK_HAS_HFCLK24M
int nrfx_clock_xo24m_init(nrfx_clock_xo24m_event_handler_t xo24m_event_handler)
{
	m_xo24m_event_handler = xo24m_event_handler;

	return 0;
}
#endif /* NRF_CLOCK_HAS_HFCLK24M */

void nrfx_clock_lfclk_irq_handler(void)
{
}

#if NRF_CLOCK_HAS_HFCLK
void nrfx_clock_hfclk_irq_handler(void)
{
}
#endif /* NRF_CLOCK_HAS_HFCLK */

#if NRF_CLOCK_HAS_XO
void nrfx_clock_xo_irq_handler(void)
{
}
#endif /* NRF_CLOCK_HAS_XO */

#if NRF_CLOCK_HAS_HFCLK24M
void nrfx_clock_xo24m_irq_handler(void)
{
}
#endif /* NRF_CLOCK_HAS_HFCLK24M */

#endif /* !defined(CONFIG_CLOCK_CONTROL_NRF) */
