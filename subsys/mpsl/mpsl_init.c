/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <irq.h>
#include <kernel.h>
#include <logging/log.h>
#include <sys/__assert.h>
#include <mpsl.h>
#include <mpsl_timeslot.h>
#include "multithreading_lock.h"

LOG_MODULE_REGISTER(mpsl_init, CONFIG_MPSL_LOG_LEVEL);

#if IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
	#define MPSL_LOW_PRIO_IRQn SWI5_IRQn
#elif IS_ENABLED(CONFIG_SOC_SERIES_NRF53X)
	#define MPSL_LOW_PRIO_IRQn EGU0_IRQn
#endif
#define MPSL_LOW_PRIO (4)

static K_SEM_DEFINE(sem_signal, 0, UINT_MAX);
static struct k_thread signal_thread_data;
static K_THREAD_STACK_DEFINE(signal_thread_stack,
			     CONFIG_MPSL_SIGNAL_STACK_SIZE);

#if CONFIG_MPSL_TIMESLOT_SESSION_COUNT > 0
#define TIMESLOT_MEM_SIZE \
	((MPSL_TIMESLOT_CONTEXT_SIZE) * \
	(CONFIG_MPSL_TIMESLOT_SESSION_COUNT))
static uint8_t __aligned(4) timeslot_context[TIMESLOT_MEM_SIZE];
#endif

static void mpsl_low_prio_irq_handler(void)
{
	k_sem_give(&sem_signal);
}

static void signal_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int errcode;

	while (true) {
		k_sem_take(&sem_signal, K_FOREVER);

		errcode = MULTITHREADING_LOCK_ACQUIRE();
		__ASSERT_NO_MSG(errcode == 0);
		mpsl_low_priority_process();
		MULTITHREADING_LOCK_RELEASE();
	}
}

ISR_DIRECT_DECLARE(mpsl_timer0_isr_wrapper)
{
	MPSL_IRQ_TIMER0_Handler();

	ISR_DIRECT_PM();


	/* We may need to reschedule in case a radio timeslot callback
	 * accesses zephyr primitives.
	 */
	return 1;
}

ISR_DIRECT_DECLARE(mpsl_rtc0_isr_wrapper)
{
	MPSL_IRQ_RTC0_Handler();

	ISR_DIRECT_PM();

	/* No need for rescheduling, because the interrupt handler
	 * does not access zephyr primitives.
	 */
	return 0;
}

ISR_DIRECT_DECLARE(mpsl_radio_isr_wrapper)
{
	MPSL_IRQ_RADIO_Handler();

	ISR_DIRECT_PM();

	/* We may need to reschedule in case a radio timeslot callback
	 * accesses zephyr primitives.
	 */
	return 1;
}

static void m_assert_handler(const char *const file, const uint32_t line)
{
	LOG_ERR("MPSL ASSERT: %s, %d", log_strdup(file), line);
	k_oops();
}

static uint8_t m_config_clock_source_get(void)
{
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
	return MPSL_CLOCK_LF_SRC_RC;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
	return MPSL_CLOCK_LF_SRC_XTAL;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
	return MPSL_CLOCK_LF_SRC_SYNTH;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING
	return MPSL_CLOCK_LF_SRC_EXT_LOW_SWING;
#elif CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING
	return MPSL_CLOCK_LF_SRC_EXT_FULL_SWING;
#else
	#error "Clock source is not supported or not defined"
	return 0;
#endif
}

static int mpsl_lib_init(struct device *dev)
{
	ARG_UNUSED(dev);
	int err = 0;
	mpsl_clock_lfclk_cfg_t clock_cfg;

	clock_cfg.source = m_config_clock_source_get();
	clock_cfg.accuracy_ppm = CONFIG_CLOCK_CONTROL_NRF_ACCURACY;

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
	clock_cfg.rc_ctiv = MPSL_RECOMMENDED_RC_CTIV;
	clock_cfg.rc_temp_ctiv = MPSL_RECOMMENDED_RC_TEMP_CTIV;
#else
	clock_cfg.rc_ctiv = 0;
	clock_cfg.rc_temp_ctiv = 0;
#endif

	err = mpsl_init(&clock_cfg, MPSL_LOW_PRIO_IRQn, m_assert_handler);
	if (err) {
		return err;
	}

#if CONFIG_MPSL_TIMESLOT_SESSION_COUNT > 0
	err = mpsl_timeslot_session_count_set((void *) timeslot_context,
			CONFIG_MPSL_TIMESLOT_SESSION_COUNT);
	if (err) {
		return err;
	}
#endif

	IRQ_DIRECT_CONNECT(TIMER0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   mpsl_timer0_isr_wrapper, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(RTC0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   mpsl_rtc0_isr_wrapper, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   mpsl_radio_isr_wrapper, IRQ_ZERO_LATENCY);

	return 0;
}

static int mpsl_signal_thread_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_thread_create(&signal_thread_data, signal_thread_stack,
			K_THREAD_STACK_SIZEOF(signal_thread_stack),
			signal_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_MPSL_THREAD_COOP_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&signal_thread_data, "MPSL signal");

	IRQ_CONNECT(MPSL_LOW_PRIO_IRQn, MPSL_LOW_PRIO,
		    mpsl_low_prio_irq_handler, NULL, 0);

	return 0;
}

SYS_INIT(mpsl_lib_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
SYS_INIT(mpsl_signal_thread_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
