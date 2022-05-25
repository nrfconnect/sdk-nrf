/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <nrf_gzll_glue.h>
#include <gzll_glue.h>
#include <nrfx_ppi.h>


#if defined(CONFIG_GAZELL_ZERO_LATENCY_IRQS)
#define GAZELL_HIGH_IRQ_FLAGS IRQ_ZERO_LATENCY
#else
#define GAZELL_HIGH_IRQ_FLAGS 0
#endif

#if defined(CONFIG_GAZELL_TIMER0)
#define GAZELL_TIMER_IRQN           TIMER0_IRQn
NRF_TIMER_Type * const nrf_gzll_timer = NRF_TIMER0;
#elif defined(CONFIG_GAZELL_TIMER1)
#define GAZELL_TIMER_IRQN           TIMER1_IRQn
NRF_TIMER_Type * const nrf_gzll_timer = NRF_TIMER1;
#elif defined(CONFIG_GAZELL_TIMER2)
#define GAZELL_TIMER_IRQN           TIMER2_IRQn
NRF_TIMER_Type * const nrf_gzll_timer = NRF_TIMER2;
#elif defined(CONFIG_GAZELL_TIMER3)
#define GAZELL_TIMER_IRQN           TIMER3_IRQn
NRF_TIMER_Type * const nrf_gzll_timer = NRF_TIMER3;
#elif defined(CONFIG_GAZELL_TIMER4)
#define GAZELL_TIMER_IRQN           TIMER4_IRQn
NRF_TIMER_Type * const nrf_gzll_timer = NRF_TIMER4;
#else
#error "Gazell timer is undefined."
#endif
IRQn_Type        const nrf_gzll_timer_irqn = GAZELL_TIMER_IRQN;

#if defined(CONFIG_GAZELL_SWI0)
#define GAZELL_SWI_IRQN             SWI0_IRQn
#elif defined(CONFIG_GAZELL_SWI1)
#define GAZELL_SWI_IRQN             SWI1_IRQn
#elif defined(CONFIG_GAZELL_SWI2)
#define GAZELL_SWI_IRQN             SWI2_IRQn
#elif defined(CONFIG_GAZELL_SWI3)
#define GAZELL_SWI_IRQN             SWI3_IRQn
#elif defined(CONFIG_GAZELL_SWI4)
#define GAZELL_SWI_IRQN             SWI4_IRQn
#elif defined(CONFIG_GAZELL_SWI5)
#define GAZELL_SWI_IRQN             SWI5_IRQn
#else
#error "Gazell software interrupt is undefined."
#endif
IRQn_Type        const nrf_gzll_swi_irqn   = GAZELL_SWI_IRQN;

__IOM uint32_t *nrf_gzll_ppi_eep0;
__IOM uint32_t *nrf_gzll_ppi_tep0;
__IOM uint32_t *nrf_gzll_ppi_eep1;
__IOM uint32_t *nrf_gzll_ppi_tep1;
__IOM uint32_t *nrf_gzll_ppi_eep2;
__IOM uint32_t *nrf_gzll_ppi_tep2;

uint32_t nrf_gzll_ppi_chen_msk_0_and_1;
uint32_t nrf_gzll_ppi_chen_msk_2;


ISR_DIRECT_DECLARE(gazell_radio_irq_handler)
{
	nrf_gzll_radio_irq_handler();

	return 0;
}

ISR_DIRECT_DECLARE(gazell_timer_irq_handler)
{
	nrf_gzll_timer_irq_handler();

	return 0;
}

static void gazell_swi_irq_handler(void *args)
{
	ARG_UNUSED(args);

	nrf_gzll_swi_irq_handler();
}

bool gzll_glue_init(void)
{
	bool is_ok = true;
	const struct device *clkctrl = DEVICE_DT_GET_ONE(nordic_nrf_clock);
	nrfx_err_t err_code;
	nrf_ppi_channel_t ppi_channel[3];
	uint8_t i;

	irq_disable(RADIO_IRQn);
	irq_disable(GAZELL_TIMER_IRQN);
	irq_disable(GAZELL_SWI_IRQN);

#if !defined(CONFIG_GAZELL_ZERO_LATENCY_IRQS)
	BUILD_ASSERT(CONFIG_GAZELL_HIGH_IRQ_PRIO < CONFIG_GAZELL_LOW_IRQ_PRIO,
		"High IRQ priority value is not lower than low IRQ priority value");
#endif

	IRQ_CONNECT(GAZELL_SWI_IRQN,
		    CONFIG_GAZELL_LOW_IRQ_PRIO,
		    gazell_swi_irq_handler,
		    NULL,
		    0);

	IRQ_DIRECT_CONNECT(GAZELL_TIMER_IRQN,
			   CONFIG_GAZELL_HIGH_IRQ_PRIO,
			   gazell_timer_irq_handler,
			   GAZELL_HIGH_IRQ_FLAGS);

	IRQ_DIRECT_CONNECT(RADIO_IRQn,
			   CONFIG_GAZELL_HIGH_IRQ_PRIO,
			   gazell_radio_irq_handler,
			   GAZELL_HIGH_IRQ_FLAGS);

	if (!device_is_ready(clkctrl)) {
		is_ok = false;
	}

	for (i = 0; i < 3; i++) {
		err_code = nrfx_ppi_channel_alloc(&ppi_channel[i]);
		if (err_code != NRFX_SUCCESS) {
			is_ok = false;
			break;
		}
	}

	if (is_ok) {
		nrf_gzll_ppi_eep0 = &NRF_PPI->CH[ppi_channel[0]].EEP;
		nrf_gzll_ppi_tep0 = &NRF_PPI->CH[ppi_channel[0]].TEP;
		nrf_gzll_ppi_eep1 = &NRF_PPI->CH[ppi_channel[1]].EEP;
		nrf_gzll_ppi_tep1 = &NRF_PPI->CH[ppi_channel[1]].TEP;
		nrf_gzll_ppi_eep2 = &NRF_PPI->CH[ppi_channel[2]].EEP;
		nrf_gzll_ppi_tep2 = &NRF_PPI->CH[ppi_channel[2]].TEP;

		nrf_gzll_ppi_chen_msk_0_and_1 = ((1 << ppi_channel[0]) |
						 (1 << ppi_channel[1]));
		nrf_gzll_ppi_chen_msk_2 = (1 << ppi_channel[2]);
	}

	return is_ok;
}

void nrf_gzll_delay_us(uint32_t usec_to_wait)
{
	k_busy_wait(usec_to_wait);
}

void nrf_gzll_request_xosc(void)
{
	z_nrf_clock_bt_ctlr_hf_request();

	/* Wait 1.5ms with 9% tolerance.
	 * 1500 * 1.09 = 1635
	 */
	k_busy_wait(1635);
}

void nrf_gzll_release_xosc(void)
{
	z_nrf_clock_bt_ctlr_hf_release();
}
