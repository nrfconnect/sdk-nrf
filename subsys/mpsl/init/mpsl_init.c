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
#include <nrfx.h>
#if defined(CONFIG_NRFX_DPPI)
#include <nrfx_dppi.h>
#endif
#if defined(CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START)
#include <hal/nrf_ipc.h>
#endif
/* CONFIG_MPSL_USE_ZEPHYR_PM needs to be changed to correct config*/
#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
#include <mpsl_pm.h>
#include <zephyr/pm/policy.h>

/* These values are still arbritrary*/
#define MAX_DELAY_SINCE_READING_PARAMS 50
#define TIME_TO_REGISTER_EVENT_IN_ZEPHYR 1000

static void mpsl_pm_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(pm_work, mpsl_pm_work_handler);
#endif

LOG_MODULE_REGISTER(mpsl_init, CONFIG_MPSL_LOG_LEVEL);

/* The following two constants are used in nrfx_glue.h for marking these PPI
 * channels and groups as occupied and thus unavailable to other modules.
 */
const uint32_t z_mpsl_used_nrf_ppi_channels = MPSL_RESERVED_PPI_CHANNELS;
const uint32_t z_mpsl_used_nrf_ppi_groups;
#if defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC) && !defined(CONFIG_SOC_SERIES_NRF54HX)
static void mpsl_calibration_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(calibration_work, mpsl_calibration_work_handler);
#endif /* CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC && !CONFIG_SOC_SERIES_NRF54HX */

extern void rtc_pretick_rtc0_isr_hook(void);

#if IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF52X)
	#if IS_ENABLED(CONFIG_NRF52_ANOMALY_109_WORKAROUND)
		BUILD_ASSERT(CONFIG_NRF52_ANOMALY_109_WORKAROUND_EGU_INSTANCE != 5,
			     "MPSL uses EGU instance 5, please use another one");
	#endif
#endif

#if !defined(CONFIG_SOC_SERIES_NRF54HX) && !defined(CONFIG_SOC_SERIES_NRF54LX)
#define MPSL_TIMER_IRQn TIMER0_IRQn
#define MPSL_RTC_IRQn RTC0_IRQn
#define MPSL_RADIO_IRQn RADIO_IRQn
#elif defined(CONFIG_SOC_SERIES_NRF54LX)
#define MPSL_TIMER_IRQn TIMER10_IRQn
#define MPSL_RTC_IRQn	GRTC_3_IRQn
#define MPSL_RADIO_IRQn RADIO_0_IRQn
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#define MPSL_TIMER_IRQn TIMER020_IRQn
#define MPSL_RTC_IRQn GRTC_2_IRQn
#define MPSL_RADIO_IRQn RADIO_0_IRQn

/* Basic build time sanity checking */
#define MPSL_RESERVED_GRTC_CHANNELS ((1U << 8) | (1U << 9) | (1U << 10) | (1U << 11) | (1U << 12))
#define MPSL_RESERVED_DPPI_SOURCE_CHANNELS (1U << 0)
#define MPSL_RESERVED_DPPI_SINK_CHANNELS (1U << 0)
#define MPSL_RESERVED_IPCT_SOURCE_CHANNELS (1U << 0)

BUILD_ASSERT(MPSL_RTC_IRQn != DT_IRQN(DT_NODELABEL(grtc)), "MPSL requires a dedicated GRTC IRQ");

#define CHECK_IRQ(val, _) (DT_IRQ_BY_IDX(DT_NODELABEL(grtc), val, irq) == MPSL_RTC_IRQn)
#define NUM_IRQS DT_NUM_IRQS(DT_NODELABEL(grtc))
/* Note: MPSL_RTC_IRQn is an enum value so this macro will be a concatenation of
 * conditions and not 0 or 1
 */
#define MPSL_IRQ_IN_DT (LISTIFY(NUM_IRQS, CHECK_IRQ, (|)))

BUILD_ASSERT(MPSL_IRQ_IN_DT, "The MPSL GRTC IRQ is not in the device tree");

BUILD_ASSERT((NRFX_CONFIG_MASK_DT(DT_NODELABEL(grtc), child_owned_channels) &
	      MPSL_RESERVED_GRTC_CHANNELS) == MPSL_RESERVED_GRTC_CHANNELS,
	     "The GRTC channels used by MPSL must not be used by zephyr");

/* check the GRTC source channels.
 * i.e. ensure something similar to this is present in the DT
 * &dppic132 {
 *   owned-channels = <0>;
 *   source-channels = <0>;
 * };
 */
#define SHIFT_DPPI_SOURCE_CHANNELS(val, _)                                                         \
	(1 << (DT_PROP_BY_IDX(DT_NODELABEL(dppic132), source_channels, val)))
#define NUM_DPPI_SOURCE_CHANNELS DT_PROP_LEN(DT_NODELABEL(dppic132), source_channels)
#define DPPI_SOURCE_CHANNELS (LISTIFY(NUM_DPPI_SOURCE_CHANNELS, SHIFT_DPPI_SOURCE_CHANNELS, (|)))

BUILD_ASSERT((DPPI_SOURCE_CHANNELS & MPSL_RESERVED_DPPI_SOURCE_CHANNELS) ==
		     MPSL_RESERVED_DPPI_SOURCE_CHANNELS,
	     "The required DPPIC channels in the GRTC domain are not reserved");

/* check the GRTC sink channels.
 * i.e. ensure something similar to this is present in the DT
 * &dppic130 {
 *   owned-channels = <0>;
 *   sink-channels = <0>;
 * };
 */
#define SHIFT_DPPI_SINK_CHANNELS(val, _)                                                           \
	(1 << (DT_PROP_BY_IDX(DT_NODELABEL(dppic130), sink_channels, val)))
#define NUM_DPPI_SINK_CHANNELS DT_PROP_LEN(DT_NODELABEL(dppic130), sink_channels)
#define DPPI_SINK_CHANNELS (LISTIFY(NUM_DPPI_SINK_CHANNELS, SHIFT_DPPI_SINK_CHANNELS, (|)))

BUILD_ASSERT((DPPI_SINK_CHANNELS & MPSL_RESERVED_DPPI_SINK_CHANNELS) ==
		     MPSL_RESERVED_DPPI_SINK_CHANNELS,
	     "The required DPPIC channels in the IPCT domain are not reserved");

/* check the IPCT source channels.
 * i.e. ensure something similar to this is present in the DT
 * &ipct130 {
 *   owned-channels = <0>;
 *   source-channel-links = <0 3 0>;
 * };
 */
#define SHIFT_IPCT_SOURCE_CHANNELS(val, _)                                                         \
	(1 << (DT_PROP_BY_IDX(DT_NODELABEL(ipct130), owned_channels, val)))
#define NUM_IPCT_SOURCE_CHANNELS DT_PROP_LEN(DT_NODELABEL(ipct130), owned_channels)
#define IPCT_SOURCE_CHANNELS (LISTIFY(NUM_IPCT_SOURCE_CHANNELS, SHIFT_IPCT_SOURCE_CHANNELS, (|)))

/* NOTE: this is not checking the source/sink allocation as the device tree property is an
 * array of triplets that is difficult to separate using macros.
 */
BUILD_ASSERT((IPCT_SOURCE_CHANNELS & MPSL_RESERVED_IPCT_SOURCE_CHANNELS) ==
		     MPSL_RESERVED_IPCT_SOURCE_CHANNELS,
	     "The required IPCT source channels are not reserved");

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

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)

#define PM_MAX_LATENCY_IDLE_US 4999999

static uint8_t	m_pm_prev_flag_value;
static bool		m_pm_latency_idle_set;
static bool		m_pm_event_is_scheduled;

#endif

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

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
static bool m_check_if_pm_work_is_needed(void)
{
	mpsl_pm_params_t params;

	mpsl_pm_params_get(&params);
	return (m_pm_prev_flag_value != params.mpsl_pm_cnt_flag);
}

static bool m_check_if_pm_latency_needs_updating(uint32_t lat_value_us)
{
	/* After event or In Event */
	return (((lat_value_us/* == PM_MAX_LATENCY_IDLE_US*/) &&
			 (m_pm_latency_idle_set == false)) ||
			((lat_value_us == 0) && (m_pm_latency_idle_set == true)));
}

static void m_update_latency_pm(uint32_t lat_value_us)
{
	if (m_check_if_pm_latency_needs_updating(lat_value_us)) {
		pm_policy_latency_request_update(&pm_policy_latency_request, lat_value_us);
		m_pm_latency_idle_set = lat_value_us ? true : false;
	}
}

static void m_pm_work(void)
{
	mpsl_pm_params_t params = {0};
	uint32_t lat_value_us   = 0;
	bool no_param_update    = mpsl_pm_params_get(&params);

	/* High prio did not change mpsl_pm_params, while we read the params. */
	if (no_param_update) {
		if (params.event_state == MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT) {
			/* No event scheduled, so set latency to restrict deepest sleep states*/
			/* Note: The core running sdc should have it's PM state definitions, like
			 * https://github.com/nrfconnect/sdk-sysctrl/blob/master/sysctrl/conf/boards/pm.dtsi#L26
			 */

			lat_value_us = PM_MAX_LATENCY_IDLE_US;
			m_update_latency_pm(lat_value_us);
			/* Note: While m_pm_latency_idle_set is also true before the event,
			 * the else already excludes that case.
			 */
			pm_policy_event_unregister(&pm_policy_events);
			m_pm_event_is_scheduled = false;
		} else if (params.event_state == MPSL_PM_EVENT_STATE_BEFORE_EVENT) {
			/* Event scheduled */
			uint64_t wakeup_latency_us = params.event_time_rel_us -
			                             MAX_DELAY_SINCE_READING_PARAMS;

			if (wakeup_latency_us > UINT32_MAX) {
				uint32_t retry_time_us = UINT32_MAX -
							 TIME_TO_REGISTER_EVENT_IN_ZEPHYR;

				k_work_schedule_for_queue(&mpsl_work_q, &pm_work,
							  K_MSEC(retry_time_us));
				return;
			}

			if (m_pm_event_is_scheduled) {
				pm_policy_event_update(&pm_policy_event, wakeup_latency_us);
			} else {
				pm_policy_event_register(&pm_policy_event, wakeup_latency_us);
			}
			m_pm_event_is_scheduled = true;
		} else {
			/* Event started */
			/* we are in and event, set latency 0*/
			lat_value_us = 0;
			m_update_latency_pm(lat_value_us);
		}
	}
	m_pm_prev_flag_value = params.mpsl_pm_cnt_flag;
}
#endif

static void mpsl_low_prio_irq_handler(const void *arg)
{
	mpsl_work_submit(&mpsl_low_prio_work);
}

static void mpsl_low_prio_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	__ASSERT_NO_MSG(errcode == 0);

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
	if (m_check_if_pm_work_is_needed()) {
		m_pm_work();
	}
#endif

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

	if (IS_ENABLED(CONFIG_SOC_NRF53_RTC_PRETICK) &&
	    IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET)) {
		rtc_pretick_rtc0_isr_hook();
	}

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
	irq_disable(MPSL_TIMER_IRQn);
	irq_disable(MPSL_RTC_IRQn);
	irq_disable(MPSL_RADIO_IRQn);
}

static void mpsl_lib_irq_connect(void)
{
	/* Ensure IRQs are disabled before attaching. */
	mpsl_lib_irq_disable();

	irq_connect_dynamic(MPSL_TIMER_IRQn, MPSL_HIGH_IRQ_PRIORITY, mpsl_timer0_isr_wrapper, NULL,
			    IRQ_CONNECT_FLAGS);
	irq_connect_dynamic(MPSL_RTC_IRQn, MPSL_HIGH_IRQ_PRIORITY, mpsl_rtc0_isr_wrapper, NULL,
			    IRQ_CONNECT_FLAGS);
	irq_connect_dynamic(MPSL_RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY, mpsl_radio_isr_wrapper, NULL,
			    IRQ_CONNECT_FLAGS);

	irq_enable(MPSL_TIMER_IRQn);
	irq_enable(MPSL_RTC_IRQn);
	irq_enable(MPSL_RADIO_IRQn);
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
	if (IS_ENABLED(CONFIG_SOC_NRF53_RTC_PRETICK) &&
	    IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET)) {
		rtc_pretick_rtc0_isr_hook();
	}
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
#if defined(CONFIG_ASSERT) && defined(CONFIG_ASSERT_VERBOSE) && !defined(CONFIG_ASSERT_NO_MSG_INFO)
	__ASSERT(false, "MPSL ASSERT: %s, %d\n", file, line);
#elif defined(CONFIG_LOG)
	LOG_ERR("MPSL ASSERT: %s, %d", file, line);
	k_oops();
#elif defined(CONFIG_PRINTK)
	printk("MPSL ASSERT: %s, %d\n", file, line);
	printk("\n");
	k_oops();
#else
	k_oops();
#endif
}
#endif /* IS_ENABLED(CONFIG_MPSL_ASSERT_HANDLER) */

#if !defined(CONFIG_SOC_SERIES_NRF54HX)
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
#endif /* !CONFIG_SOC_SERIES_NRF54HX */

#if defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC) && !defined(CONFIG_SOC_SERIES_NRF54HX)
static void mpsl_calibration_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	mpsl_calibration_timer_handle();

	k_work_schedule_for_queue(&mpsl_work_q, &calibration_work,
				  K_MSEC(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD));
}
#endif /* CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC && !CONFIG_SOC_SERIES_NRF54HX */

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
static void mpsl_pm_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	m_pm_work();
}

static void m_pm_init(void)
{
	mpsl_pm_params_t params = {0};
	/* So that PM doesn't shut us off power when we are idle.*/
	/* see min. residency times: https://github.com/nrfconnect/sdk-sysctrl/blob/master/sysctrl/conf/pm.overlay#L19*/
	/* we want max sleep?? -> PM_MAX_LATENCY_IDLE_US < 5000000us*/
	pm_policy_latency_request_add(&pm_policy_latency_request, PM_MAX_LATENCY_IDLE_US);
	m_pm_latency_idle_set = true;

	mpsl_pm_init();
	mpsl_pm_params_get(&params);
	m_pm_prev_flag_value = params.mpsl_pm_cnt_flag;
	m_pm_event_is_scheduled = false;
}
#endif

static int32_t mpsl_lib_init_internal(void)
{
	int err = 0;
	mpsl_clock_lfclk_cfg_t clock_cfg;

#ifdef CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START
	nrf_ipc_send_config_set(NRF_IPC,
		CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START_CHANNEL,
		(1UL << CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START_CHANNEL));
	mpsl_clock_task_trigger_on_rtc_start_set(
		(uint32_t)&NRF_IPC->TASKS_SEND[CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START_CHANNEL]);
#endif

	/* TODO: Clock config should be adapted in the future to new architecture. */
#if !defined(CONFIG_SOC_SERIES_NRF54HX)
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
#else
	/* For now just set the values to 0 to avoid "use of uninitialized variable" warnings.
	 * MPSL assumes the clocks are always available and does currently not implement
	 * clock handling on these platforms. The LFCLK is expected to have an accuracy of
	 * 500ppm or better regardless of the value passed in clock_cfg.
	 */
	memset(&clock_cfg, 0, sizeof(clock_cfg));
#endif

	err = mpsl_init(&clock_cfg, CONFIG_MPSL_LOW_PRIO_IRQN, m_assert_handler);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_SOC_NRF_FORCE_CONSTLAT)) {
		mpsl_pan_rfu();
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

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
	m_pm_init();
#endif

#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
	/* Ensure IRQs are disabled before attaching. */
	mpsl_lib_irq_disable();

	/* We may need to reschedule in case a radio timeslot callback
	 * accesses Zephyr primitives.
	 * The RTC0 interrupt handler does not access zephyr primitives,
	 * however, as this decision needs to be made during build-time,
	 * rescheduling is performed to account for user-provided handlers.
	 */
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(MPSL_TIMER_IRQn, MPSL_HIGH_IRQ_PRIORITY, IRQ_CONNECT_FLAGS,
				       reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(MPSL_RTC_IRQn, MPSL_HIGH_IRQ_PRIORITY, IRQ_CONNECT_FLAGS,
				       reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(MPSL_RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY, IRQ_CONNECT_FLAGS,
				       reschedule);

	mpsl_lib_irq_connect();
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
	IRQ_DIRECT_CONNECT(MPSL_TIMER_IRQn, MPSL_HIGH_IRQ_PRIORITY, mpsl_timer0_isr_wrapper,
			   IRQ_CONNECT_FLAGS);
	IRQ_DIRECT_CONNECT(MPSL_RTC_IRQn, MPSL_HIGH_IRQ_PRIORITY, mpsl_rtc0_isr_wrapper,
			   IRQ_CONNECT_FLAGS);
	IRQ_DIRECT_CONNECT(MPSL_RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY, mpsl_radio_isr_wrapper,
			   IRQ_CONNECT_FLAGS);
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

	IRQ_CONNECT(CONFIG_MPSL_LOW_PRIO_IRQN, MPSL_LOW_PRIO,
		    mpsl_low_prio_irq_handler, NULL, 0);

#if defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC) && !defined(CONFIG_SOC_SERIES_NRF54HX)
	k_work_schedule_for_queue(&mpsl_work_q, &calibration_work,
				  K_MSEC(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD));
#endif /* CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC && !CONFIG_SOC_SERIES_NRF54HX */

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
