/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../cmdma.h"
#include "../../crypmasterregs.h"
#include "../../hw.h"
#include "hal/nrf_cracen.h"
#include <cracen/interrupts.h>
#include <cracen/statuscodes.h>
#include <security/cracen.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <silexpk/core.h>
#include <nrf_security_events.h>


NRF_SECURITY_EVENT_DEFINE(cracen_irq_event_for_cryptomaster)
NRF_SECURITY_EVENT_DEFINE(cracen_irq_event_for_pke)

void cracen_interrupts_init(void)
{
	nrf_security_event_init(cracen_irq_event_for_cryptomaster);
	nrf_security_event_init(cracen_irq_event_for_pke);

	IRQ_CONNECT(CRACEN_IRQn, 0, cracen_isr_handler, NULL, 0);
}

#ifdef __NRF_TFM__
void CRACEN_IRQHandler(void)
{
	cracen_isr_handler(NULL);
}
#else
/* On Zephyr the IRQ_CONNECT infrastructure is given the
 * cracen_isr_handler function in the cracen_init function.
 */
#endif

void cracen_isr_handler(void *i)
{
	if (nrf_cracen_event_check(NRF_CRACEN, NRF_CRACEN_EVENT_CRYPTOMASTER)) {
		uint32_t crypto_master_status = sx_rdreg(REG_INT_STATRAW);

		/* Clear interrupts in CryptoMaster. */
		sx_wrreg(REG_INT_STATCLR, ~0);

		/* Clear Cracen wrapper interrupts.*/
		nrf_cracen_event_clear(NRF_CRACEN, NRF_CRACEN_EVENT_CRYPTOMASTER);

		/* Signal waiting threads. */
		nrf_security_event_set(cracen_irq_event_for_cryptomaster, crypto_master_status);

	} else if (nrf_cracen_event_check(NRF_CRACEN, NRF_CRACEN_EVENT_PKE_IKG)) {
		/* Clear interrupts in PKE. */
		sx_clear_interrupt(sx_get_current_req());

		/* Clear Cracen wrapper interrupts.*/
		nrf_cracen_event_clear(NRF_CRACEN, NRF_CRACEN_EVENT_PKE_IKG);

		/* Signal waiting threads. */
		nrf_security_event_set(cracen_irq_event_for_pke, 1);
	} else {
		__ASSERT(0, "Unhandled interrupt. ");
	}
}

static uint32_t cracen_wait_for_interrupt(nrf_security_event_t event)
{
	uint32_t ret = nrf_security_event_wait(event, 0xFFFFFFFF);

	/* sx_hw_reserve, sx_cmdma_release_hw, and sx_pk_acquire_req has
	 * assured single usage of CRACEN so there should be no race
	 * condition between reading this event and clearing it.
	 */
	nrf_security_event_clear(event, 0xFFFFFFFF);

	return ret;
}

uint32_t cracen_wait_for_cm_interrupt(void)
{
#if !defined(__NRF_TFM__)
	if (k_is_pre_kernel()) {
		/* Scheduling is not available pre-kernel. Initialization may
		 * run PRNG at this point.
		 */
		return 0;
	}
#endif

	return cracen_wait_for_interrupt(cracen_irq_event_for_cryptomaster);
}

uint32_t cracen_wait_for_pke_interrupt(void)
{
	return cracen_wait_for_interrupt(cracen_irq_event_for_pke);
}
