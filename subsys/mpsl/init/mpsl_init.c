/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <mpsl.h>
#include <mpsl_timeslot.h>
#include <mpsl/mpsl_assert.h>
#include <mpsl/mpsl_work.h>
#include "multithreading_lock.h"
#if defined(CONFIG_NRFX_DPPI)
#include <nrfx_dppi.h>
#endif

LOG_MODULE_REGISTER(mpsl_init, CONFIG_MPSL_LOG_LEVEL);

/* The following two constants are used in nrfx_glue.h for marking these PPI
 * channels and groups as occupied and thus unavailable to other modules.
 */
const uint32_t z_mpsl_used_nrf_ppi_channels = MPSL_RESERVED_PPI_CHANNELS;
const uint32_t z_mpsl_used_nrf_ppi_groups;

#if IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF52X)
	#define MPSL_LOW_PRIO_IRQn SWI5_IRQn
#elif IS_ENABLED(CONFIG_SOC_SERIES_NRF53X)
	#define MPSL_LOW_PRIO_IRQn SWI0_IRQn
#endif
#define MPSL_LOW_PRIO (4)

#if IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)
#define IRQ_CONNECT_FLAGS IRQ_ZERO_LATENCY
#else
#define IRQ_CONNECT_FLAGS 0
#endif

static struct k_work mpsl_low_prio_work;
struct k_work_q mpsl_work_q;
static K_THREAD_STACK_DEFINE(mpsl_work_stack, CONFIG_MPSL_WORK_STACK_SIZE);

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

static void mpsl_low_prio_irq_handler(const void *arg)
{
	k_work_submit_to_queue(&mpsl_work_q, &mpsl_low_prio_work);
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

#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
static void mpsl_timer0_isr_wrapper(const void *args)
{
	ARG_UNUSED(args);

	MPSL_IRQ_TIMER0_Handler();

	ISR_DIRECT_PM();
}

static void mpsl_rtc0_isr_wrapper(const void *args)
{
	ARG_UNUSED(args);

	MPSL_IRQ_RTC0_Handler();

	ISR_DIRECT_PM();
}

static void mpsl_radio_isr_wrapper(const void *args)
{
	ARG_UNUSED(args);

	MPSL_IRQ_RADIO_Handler();

	ISR_DIRECT_PM();
}

static void mpsl_lib_irq_disable(void)
{
	irq_disable(TIMER0_IRQn);
	irq_disable(RTC0_IRQn);
	irq_disable(RADIO_IRQn);
}

static void mpsl_lib_irq_connect(void)
{
	/* Ensure IRQs are disabled before attaching. */
	mpsl_lib_irq_disable();

	irq_connect_dynamic(TIMER0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			mpsl_timer0_isr_wrapper, NULL, IRQ_CONNECT_FLAGS);
	irq_connect_dynamic(RTC0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			mpsl_rtc0_isr_wrapper, NULL, IRQ_CONNECT_FLAGS);
	irq_connect_dynamic(RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			mpsl_radio_isr_wrapper, NULL, IRQ_CONNECT_FLAGS);

	irq_enable(TIMER0_IRQn);
	irq_enable(RTC0_IRQn);
	irq_enable(RADIO_IRQn);
}
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
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
#endif /* IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */

#if IS_ENABLED(CONFIG_MPSL_ASSERT_HANDLER)
void m_assert_handler(const char *const file, const uint32_t line)
{
	mpsl_assert_handle((char *) file, line);
}

#else /* !IS_ENABLED(CONFIG_MPSL_ASSERT_HANDLER) */
static void m_assert_handler(const char *const file, const uint32_t line)
{
	LOG_ERR("MPSL ASSERT: %s, %d", file, line);
	k_oops();
}
#endif /* IS_ENABLED(CONFIG_MPSL_ASSERT_HANDLER) */

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

static int32_t mpsl_lib_init_internal(void)
{
	int err = 0;
	mpsl_clock_lfclk_cfg_t clock_cfg;

	clock_cfg.source = m_config_clock_source_get();
	clock_cfg.accuracy_ppm = CONFIG_CLOCK_CONTROL_NRF_ACCURACY;
	clock_cfg.skip_wait_lfclk_started =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT);

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
	BUILD_ASSERT(IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION),
		    "MPSL requires clock calibration to be enabled when RC is used as LFCLK");

	/* clock_cfg.rc_ctiv is given in 1/4 seconds units.
	 * CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD is given in ms. */
	clock_cfg.rc_ctiv = (CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD * 4 / 1000);
	clock_cfg.rc_temp_ctiv = CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP + 1;
	BUILD_ASSERT(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF == 2,
		     "MPSL always uses a temperature diff threshold of 0.5 degrees");
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

	return 0;
}

static int mpsl_lib_init_sys(void)
{
	int err = 0;

	err = mpsl_lib_init_internal();
	if (err) {
		return err;
	}

#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
	/* Ensure IRQs are disabled before attaching. */
	mpsl_lib_irq_disable();

	/* We may need to reschedule in case a radio timeslot callback
	 * accesses Zephyr primitives.
	 * The RTC0 interrupt handler does not access zephyr primitives,
	 * however, as this decision needs to be made during build-time,
	 * rescheduling is performed to account for user-provided handlers.
	 */
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(TIMER0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			IRQ_CONNECT_FLAGS, reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RTC0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			IRQ_CONNECT_FLAGS, reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			IRQ_CONNECT_FLAGS, reschedule);

	mpsl_lib_irq_connect();
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
	IRQ_DIRECT_CONNECT(TIMER0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   mpsl_timer0_isr_wrapper, IRQ_CONNECT_FLAGS);
	IRQ_DIRECT_CONNECT(RTC0_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   mpsl_rtc0_isr_wrapper, IRQ_CONNECT_FLAGS);
	IRQ_DIRECT_CONNECT(RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY,
			   mpsl_radio_isr_wrapper, IRQ_CONNECT_FLAGS);
#endif /* IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */

	return 0;
}

static int mpsl_low_prio_init(void)
{

	k_work_queue_start(&mpsl_work_q, mpsl_work_stack,
			   K_THREAD_STACK_SIZEOF(mpsl_work_stack),
			   K_PRIO_COOP(CONFIG_MPSL_THREAD_COOP_PRIO), NULL);
	k_thread_name_set(&mpsl_work_q.thread, "MPSL Work");
	k_work_init(&mpsl_low_prio_work, mpsl_low_prio_work_handler);

	IRQ_CONNECT(MPSL_LOW_PRIO_IRQn, MPSL_LOW_PRIO,
		    mpsl_low_prio_irq_handler, NULL, 0);

	return 0;
}

int32_t mpsl_lib_init(void)
{
#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
	int err = 0;

	err = mpsl_lib_init_internal();
	if (err) {
		return err;
	}

	mpsl_lib_irq_connect();

	return 0;
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
	return -NRF_EPERM;
#endif /* IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
}

int32_t mpsl_lib_uninit(void)
{
#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
	mpsl_lib_irq_disable();

	mpsl_uninit();

	return 0;
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
	return -NRF_EPERM;
#endif /* IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
}

SYS_INIT(mpsl_lib_init_sys, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
SYS_INIT(mpsl_low_prio_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
