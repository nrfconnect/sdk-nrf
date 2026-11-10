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

#include <zephyr/devicetree.h>
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
 * LFCLK with MPSL (split lfclk driver):
 *
 * 1. Zephyr lfclk init (PRE_KERNEL): nrfx_clock_lfclk_init() stores the Zephyr
 *    event handler; onoff keeps LF off until a client requests it. No HW start.
 *
 * 2. mpsl_init(): MPSL starts LFCLK in clock_ctrl (m_clock_ctrl_lfclk_start).
 *    With skip_wait_lfclk_started (e.g. GRTC defer LFXO + WAIT_FOR_AVAILABILITY),
 *    init does not block on LFSTAT; LFCLK may still be starting when the OS runs.
 *
 * 3. LF onoff request: lfclk_start() → nrfx_clock_lfclk_start() below.
 *
 *    On SoCs with GRTC in devicetree (except nRF54H20): mpsl_clock_lfclk_request()
 *    registers for LFCLKSTARTED in MPSL. EVENTS_LFCLKSTARTED is handled in
 *    MPSL_IRQ_CLOCK_Handler; mpsl_lfclk_started_callback forwards
 *    NRFX_CLOCK_LFCLK_EVT_LFCLK_STARTED to the handler registered in step 1.
 *    That completes the onoff notify (e.g. GRTC grtc_post_init() deferred LFXO
 *    path: wait for LFXO ready, then run the safe GRTC clock-source switch).
 *
 *    Other builds: nrfx_clock_lfclk_start() still signals LFCLK started
 *    immediately so onoff can finish without a second HW wait (LF already
 *    owned and started by MPSL init).
 *
 * z_nrf_clock_control_lf_on() may still lfclk_spinwait() per SYSTEM_CLOCK_* policy
 * after onoff; that is separate from the LFCLKSTARTED notify above.
 *
 * LFCLKSTARTED and LF RC CAL_DONE from HW are consumed in MPSL_IRQ_CLOCK_Handler();
 * nrfx_clock_lfclk_irq_handler() is empty. Zephyr calibration module is off when
 * CLOCK_CONTROL_NRF_FORCE_ALT is set.
 */
#if !defined(CONFIG_CLOCK_CONTROL_NRF) && DT_NODE_EXISTS(DT_NODELABEL(grtc)) &&                  \
	!defined(CONFIG_SOC_NRF54H20)
static void mpsl_lfclk_started_callback(mpsl_clock_evt_type_t evt_type)
{
	if ((evt_type == MPSL_CLOCK_EVT_LFCLK_STARTED) && (m_lfclk_event_handler != NULL)) {
		m_lfclk_event_handler(NRFX_CLOCK_LFCLK_EVT_LFCLK_STARTED);
	}
}

#define CLOCK_NODE_LFCLK DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_lfclk)

#if (!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, rc) &&                                           \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, xtal) &&                                      \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth) &&                                     \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_low_swing) &&                             \
	!DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_full_swing))
#error "Unsupported LFCLK source configured in devicetree"
#endif

#if DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, rc)
#define MPSL_LFCLK_SRC MPSL_CLOCK_LF_SRC_RC
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, xtal)
#define MPSL_LFCLK_SRC MPSL_CLOCK_LF_SRC_XTAL
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, synth)
#define MPSL_LFCLK_SRC MPSL_CLOCK_LF_SRC_SYNTH
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_low_swing)
#define MPSL_LFCLK_SRC MPSL_CLOCK_LF_SRC_EXT_LOW_SWING
#elif DT_ENUM_HAS_VALUE(CLOCK_NODE_LFCLK, k32src, ext_full_swing)
#define MPSL_LFCLK_SRC MPSL_CLOCK_LF_SRC_EXT_FULL_SWING
#endif
#endif /* !CONFIG_CLOCK_CONTROL_NRF && GRTC SoC (not nRF54H20) */

void nrfx_clock_lfclk_start(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	m_event_handler(NRFX_CLOCK_EVT_LFCLK_STARTED);
#else
#if DT_NODE_EXISTS(DT_NODELABEL(grtc)) && !defined(CONFIG_SOC_NRF54H20)
	int err;

	err = mpsl_clock_lfclk_request(MPSL_LFCLK_SRC, mpsl_lfclk_started_callback);
	if (err < 0) {
		LOG_ERR("mpsl_clock_lfclk_request failed: %d", err);
		__ASSERT_NO_MSG(false);
	}
#else
	if (m_lfclk_event_handler != NULL) {
		m_lfclk_event_handler(NRFX_CLOCK_LFCLK_EVT_LFCLK_STARTED);
	}
#endif /* DT_NODELABEL(grtc) && !CONFIG_SOC_NRF54H20 */
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

#if NRF_LFRC_HAS_CALIBRATION
	IRQ_CONNECT(LFRC_IRQn, DT_IRQ(DT_NODELABEL(clock), priority), nrfx_isr,
		    nrfx_clock_irq_handler, 0);
#endif

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
