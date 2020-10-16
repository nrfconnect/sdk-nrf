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

static void mpsl_low_prio_work_handler(struct k_work *item);

static K_WORK_DEFINE(mpsl_low_prio_work, mpsl_low_prio_work_handler);

#if IS_ENABLED(CONFIG_SOC_SERIES_NRF52X)
	#define MPSL_LOW_PRIO_IRQn SWI5_IRQn
#elif IS_ENABLED(CONFIG_SOC_SERIES_NRF53X)
	#define MPSL_LOW_PRIO_IRQn EGU0_IRQn
#endif
#define MPSL_LOW_PRIO (4)

#define MPSL_TIMESLOT_SESSION_COUNT (\
	CONFIG_MPSL_TIMESLOT_SESSION_COUNT + \
	CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL_TIMESLOT_SESSION_COUNT)
BUILD_ASSERT(MPSL_TIMESLOT_SESSION_COUNT <= MPSL_TIMESLOT_CONTEXT_COUNT_MAX,
	     "Too many timeslot sessions");

#if MPSL_TIMESLOT_SESSION_COUNT > 0
#define TIMESLOT_MEM_SIZE \
	((MPSL_TIMESLOT_CONTEXT_SIZE) * \
	(MPSL_TIMESLOT_SESSION_COUNT))
static uint8_t __aligned(4) timeslot_context[TIMESLOT_MEM_SIZE];
#endif

static void mpsl_low_prio_irq_handler(void)
{
	k_work_submit(&mpsl_low_prio_work);
}

static void mpsl_low_prio_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	__ASSERT_NO_MSG(errcode == 0);
	mpsl_low_priority_process();
	MULTITHREADING_LOCK_RELEASE();
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

static int mpsl_lib_init(const struct device *dev)
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

#if MPSL_TIMESLOT_SESSION_COUNT > 0
	err = mpsl_timeslot_session_count_set((void *) timeslot_context,
			MPSL_TIMESLOT_SESSION_COUNT);
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
	IRQ_CONNECT(MPSL_LOW_PRIO_IRQn, MPSL_LOW_PRIO,
		    mpsl_low_prio_irq_handler, NULL, 0);

	return 0;
}

SYS_INIT(mpsl_lib_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
