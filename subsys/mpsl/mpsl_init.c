/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <irq.h>
#include <kernel.h>
#include <mpsl.h>

#define MPSL_LOW_PRIO_IRQ SWI4_IRQn
#define MPSL_LOW_PRIO (4)


static void mpsl_low_prio_irq_handler(void)
{
	mpsl_low_priority_process();
}

static void m_assert_handler(const char *const file, const u32_t line)
{
	k_oops();
}


uint8_t m_config_clock_source_get(void)
{
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
	return MPSL_CLOCK_LF_SRC_RC;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
	return MPSL_CLOCK_LF_SRC_XTAL;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
	return MPSL_CLOCK_LF_SRC_SYNTH;
#else
	#error "Clock source is not supported or not defined"
	return 0;
#endif
}

static int mpsl_lib_init(struct device *dev)
{
	ARG_UNUSED(dev);

	int err = 0;

	mpsl_clock_lf_cfg_t clock_cfg;

	clock_cfg.source = m_config_clock_source_get();
	clock_cfg.rc_ctiv = MPSL_RECOMMENDED_RC_CTIV;
	clock_cfg.rc_temp_ctiv = MPSL_RECOMMENDED_RC_TEMP_CTIV;

	err = mpsl_init(&clock_cfg, MPSL_LOW_PRIO_IRQ, m_assert_handler);
	if (err) {
		return err;
	}

	IRQ_DIRECT_CONNECT(TIMER0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   MPSL_IRQ_TIMER0_Handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(RTC0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   MPSL_IRQ_RTC0_Handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   MPSL_IRQ_RADIO_Handler, IRQ_ZERO_LATENCY);
	IRQ_CONNECT(MPSL_LOW_PRIO_IRQ, MPSL_LOW_PRIO,
		    mpsl_low_prio_irq_handler, NULL, 0);
	irq_enable(MPSL_LOW_PRIO_IRQ);

	return 0;
}

SYS_INIT(mpsl_lib_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
