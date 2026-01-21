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
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <mpsl.h>
#include <mpsl_timeslot.h>
#include <mpsl/mpsl_assert.h>
#include <mpsl/mpsl_work.h>
#include "multithreading_lock.h"
#include <nrfx.h>
#if defined(NRF_TRUSTZONE_NONSECURE)
#include "tfm_platform_api.h"
#include "tfm_ioctl_core_api.h"
#endif
#if IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF54LX)
#include <nrfx_power.h>
#endif
#if defined(CONFIG_SOC_SERIES_NRF54H)
#include <hal/nrf_dppi.h>
#endif
#if defined(CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START)
#include <hal/nrf_ipc.h>
#endif

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
#include "../pm/mpsl_pm_utils.h"
#endif

#if IS_ENABLED(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
#include "../clock_ctrl/mpsl_clock_ctrl.h"
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

LOG_MODULE_REGISTER(mpsl_init, CONFIG_MPSL_LOG_LEVEL);

#if defined(CONFIG_MPSL_CALIBRATION_PERIOD)
static void mpsl_calibration_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(calibration_work, mpsl_calibration_work_handler);
#endif /* CONFIG_MPSL_CALIBRATION_PERIOD */

extern void rtc_pretick_rtc0_isr_hook(void);

#if IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF52X)
	#if IS_ENABLED(CONFIG_NRF52_ANOMALY_109_WORKAROUND)
		BUILD_ASSERT(CONFIG_NRF52_ANOMALY_109_WORKAROUND_EGU_INSTANCE != 5,
			     "MPSL uses EGU instance 5, please use another one");
	#endif
#endif

#if IS_ENABLED(CONFIG_COUNTER)
#if IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF52X) || IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET)
BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rtc0)),
	     "MPSL reserves RTC0 on this SoC.");
BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(timer0)),
	     "MPSL reserves TIMER0 on this SoC.");
#elif IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF54LX) || IS_ENABLED(CONFIG_SOC_SERIES_NRF71)
BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(timer10)),
	     "MPSL reserves TIMER10 on this SoC.");
#elif IS_ENABLED(CONFIG_SOC_SERIES_NRF54H)
BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(timer020)),
	     "MPSL reserves TIMER020 on this SoC.");
#else
#error
#endif
#endif /* IS_ENABLED(CONFIG_COUNTER) */

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define MPSL_TIMER_IRQn TIMER0_IRQn
#define MPSL_RTC_IRQn RTC0_IRQn
#define MPSL_RADIO_IRQn RADIO_IRQn
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF71)
#define MPSL_TIMER_IRQn TIMER10_IRQn
#define MPSL_RTC_IRQn GRTC_3_IRQn
#define MPSL_RADIO_IRQn RADIO_0_IRQn
#elif defined(CONFIG_SOC_SERIES_NRF54H)
#define MPSL_TIMER_IRQn TIMER020_IRQn
#define MPSL_RTC_IRQn GRTC_2_IRQn
#define MPSL_RADIO_IRQn RADIO_0_IRQn
#endif

#if defined(CONFIG_SOC_SERIES_NRF54H)
/* Basic build time sanity checking */
#define MPSL_RESERVED_GRTC_CHANNELS ((1U << 8) | (1U << 9) | (1U << 10) | (1U << 11) | (1U << 12))
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF71)
#define MPSL_RESERVED_GRTC_CHANNELS ((1U << 7) | (1U << 8) | (1U << 9) | (1U << 10) | (1U << 11))
#endif

#if defined(CONFIG_SOC_SERIES_NRF54H) || \
	defined(CONFIG_SOC_COMPATIBLE_NRF54LX) || \
	defined(CONFIG_SOC_SERIES_NRF71)

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
#endif

#if defined(CONFIG_SOC_SERIES_NRF54H)
#define MPSL_RESERVED_IPCT_SOURCE_CHANNELS (1U << 0)
#define MPSL_RESERVED_DPPI_SOURCE_CHANNELS (1U << 0)
#define MPSL_RESERVED_DPPI_SINK_CHANNELS (1U << 0)
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

#if defined(CONFIG_SOC_SERIES_NRF54L)
BUILD_ASSERT(NRF_CONFIG_CPU_FREQ_MHZ == 128, "Currently mpsl only works when frequency is 128MHz");
#endif

#if IS_ENABLED(CONFIG_NRF_GRTC_TIMER) && !defined(CONFIG_SOC_SERIES_NRF54H)
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_GRTC_TIMER_AUTO_KEEP_ALIVE),
	     "MPSL requires NRF_GRTC_TIMER_AUTO_KEEP_ALIVE to be enabled when using GRTC timer");
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
	mpsl_work_submit(&mpsl_low_prio_work);
}

static void mpsl_low_prio_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	int errcode;

	errcode = MULTITHREADING_LOCK_ACQUIRE();
	__ASSERT_NO_MSG(errcode == 0);

	mpsl_low_priority_process();

#if IS_ENABLED(CONFIG_MPSL_USE_ZEPHYR_PM)
	mpsl_pm_utils_work_handler();
#endif

	MULTITHREADING_LOCK_RELEASE();
}

#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
static void mpsl_timer0_isr_wrapper(const void *args)
{
	ARG_UNUSED(args);

	MPSL_IRQ_TIMER0_Handler();
}

static void mpsl_rtc0_isr_wrapper(const void *args)
{
	ARG_UNUSED(args);

	if (IS_ENABLED(CONFIG_SOC_NRF53_RTC_PRETICK) &&
	    IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET)) {
		rtc_pretick_rtc0_isr_hook();
	}

	MPSL_IRQ_RTC0_Handler();
}

static void mpsl_radio_isr_wrapper(const void *args)
{
	ARG_UNUSED(args);

	MPSL_IRQ_RADIO_Handler();
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

	return 0;
}

ISR_DIRECT_DECLARE(mpsl_rtc0_isr_wrapper)
{
	if (IS_ENABLED(CONFIG_SOC_NRF53_RTC_PRETICK) &&
	    IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET)) {
		rtc_pretick_rtc0_isr_hook();
	}
	MPSL_IRQ_RTC0_Handler();

	return 0;
}

ISR_DIRECT_DECLARE(mpsl_radio_isr_wrapper)
{
	MPSL_IRQ_RADIO_Handler();

	return 0;
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
	volatile char assert_file_id[11] = { 0 };
	volatile uint32_t assert_line = line;

	strncpy((char *)assert_file_id, file, sizeof(assert_file_id) - 1);

#if defined(CONFIG_ASSERT) && defined(CONFIG_ASSERT_VERBOSE) && !defined(CONFIG_ASSERT_NO_MSG_INFO)
	__ASSERT(false, "MPSL ASSERT: %s, %d\n", (char *)assert_file_id, assert_line);
#elif defined(CONFIG_LOG)
	LOG_ERR("MPSL ASSERT: %s, %d", (char *)assert_file_id, assert_line);
	k_oops();
#elif defined(CONFIG_PRINTK)
	printk("MPSL ASSERT: %s, %d\n", (char *)assert_file_id, assert_line);
	printk("\n");
	k_oops();
#else
	(void)assert_line;
	k_oops();
#endif
}
#endif /* IS_ENABLED(CONFIG_MPSL_ASSERT_HANDLER) */

#if !defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
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
#endif /* !CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if defined(CONFIG_MPSL_CALIBRATION_PERIOD)
static atomic_t do_calibration;

static void mpsl_calibration_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!atomic_get(&do_calibration)) {
		return;
	}

	mpsl_calibration_timer_handle();

	mpsl_work_schedule(&calibration_work,
			   K_MSEC(CONFIG_MPSL_CALIBRATION_PERIOD));
}
#endif /* CONFIG_MPSL_CALIBRATION_PERIOD */

static int32_t mpsl_lib_init_internal(void)
{
	int err = 0;
#if !defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
	mpsl_clock_lfclk_cfg_t clock_cfg;
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if defined(CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START)
	nrf_ipc_send_config_set(NRF_IPC,
		CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START_CHANNEL,
		(1UL << CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START_CHANNEL));
	mpsl_clock_task_trigger_on_rtc_start_set(
		(uint32_t)&NRF_IPC->TASKS_SEND[CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START_CHANNEL]);
#endif /* CONFIG_MPSL_TRIGGER_IPC_TASK_ON_RTC_START */

	/* TODO: Clock config should be adapted in the future to new architecture. */
#if !defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
	clock_cfg.source = m_config_clock_source_get();
	clock_cfg.accuracy_ppm = CONFIG_CLOCK_CONTROL_NRF_ACCURACY;
	clock_cfg.skip_wait_lfclk_started =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT);

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
	BUILD_ASSERT(IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION),
		    "MPSL requires clock calibration to be enabled when RC is used as LFCLK");

	/* clock_cfg.rc_ctiv is given in 1/4 seconds units.
	 * CONFIG_MPSL_CALIBRATION_PERIOD is given in ms.
	 */
	clock_cfg.rc_ctiv = (CONFIG_MPSL_CALIBRATION_PERIOD * 4 / 1000);
	clock_cfg.rc_temp_ctiv = CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP + 1;
	BUILD_ASSERT(CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF == 2,
		     "MPSL always uses a temperature diff threshold of 0.5 degrees");
#else
	clock_cfg.rc_ctiv = 0;
	clock_cfg.rc_temp_ctiv = 0;
#endif /* CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC */
#endif /* !CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
	err = mpsl_clock_ctrl_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if defined(CONFIG_MPSL_USE_ZEPHYR_PM)
	err = mpsl_pm_utils_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_MPSL_USE_ZEPHYR_PM */

#if defined(CONFIG_SOC_SERIES_NRF54H)
	/* Secure domain no longer enables DPPI channels for local domains,
	 * MPSL now has to enable the ones it uses.
	 */
	nrf_dppi_channels_enable(NRF_DPPIC130, DPPI_SINK_CHANNELS);
	nrf_dppi_channels_enable(NRF_DPPIC132, DPPI_SOURCE_CHANNELS);
#endif
#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
	err = mpsl_init(NULL, CONFIG_MPSL_LOW_PRIO_IRQN, m_assert_handler);
#else
	err = mpsl_init(&clock_cfg, CONFIG_MPSL_LOW_PRIO_IRQN, m_assert_handler);
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */
	if (err) {
		return err;
	}

#if !defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
#if defined(CONFIG_CLOCK_CONTROL_NRF) && DT_NODE_EXISTS(DT_NODELABEL(hfxo))
	mpsl_clock_hfclk_latency_set(z_nrf_clock_bt_ctlr_hf_get_startup_time_us());
#else
	mpsl_clock_hfclk_latency_set(CONFIG_MPSL_HFCLK_LATENCY);
#endif /* CONFIG_CLOCK_CONTROL_NRF && DT_NODE_EXISTS(DT_NODELABEL(hfxo)) */
#endif /* !CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if MPSL_TIMESLOT_SESSION_COUNT > 0
	err = mpsl_timeslot_session_count_set((void *) timeslot_context,
			MPSL_TIMESLOT_SESSION_COUNT);
	if (err) {
		return err;
	}
#endif /* MPSL_TIMESLOT_SESSION_COUNT > 0 */
#if defined(NRF_TRUSTZONE_NONSECURE) && \
	defined(CONFIG_MPSL_FORCE_RRAM_ON_ALL_THE_TIME)
	uint32_t result_out;
	uint32_t result = tfm_platform_mem_write32((uint32_t)&NRF_RRAMC_S->POWER.LOWPOWERCONFIG,
		RRAMC_POWER_LOWPOWERCONFIG_MODE_Standby << RRAMC_POWER_LOWPOWERCONFIG_MODE_Pos,
		RRAMC_POWER_LOWPOWERCONFIG_MODE_Msk,
		&result_out);
	(void) result_out;
	(void) result;
	  __ASSERT(result == TFM_PLATFORM_ERR_SUCCESS,
			   "tfm_platform_mem_write32 failed %i\n", result);
	  __ASSERT(result_out == 0, "tfm_platform_mem_write32 failed %i\n", result_out);
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

	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(MPSL_TIMER_IRQn, MPSL_HIGH_IRQ_PRIORITY, IRQ_CONNECT_FLAGS,
				       no_reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(MPSL_RTC_IRQn, MPSL_HIGH_IRQ_PRIORITY, IRQ_CONNECT_FLAGS,
				       no_reschedule);
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(MPSL_RADIO_IRQn, MPSL_HIGH_IRQ_PRIORITY, IRQ_CONNECT_FLAGS,
				       no_reschedule);

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

#if defined(CONFIG_MPSL_CALIBRATION_PERIOD)
	atomic_set(&do_calibration, 1);
	mpsl_work_schedule(&calibration_work,
			   K_MSEC(CONFIG_MPSL_CALIBRATION_PERIOD));
#endif /* CONFIG_MPSL_CALIBRATION_PERIOD */

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

#if defined(CONFIG_MPSL_CALIBRATION_PERIOD)
	atomic_set(&do_calibration, 1);
	mpsl_work_schedule(&calibration_work,
			   K_MSEC(CONFIG_MPSL_CALIBRATION_PERIOD));
#endif /* CONFIG_MPSL_CALIBRATION_PERIOD */

	return 0;
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
	return -NRF_EPERM;
#endif /* IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
}

int32_t mpsl_lib_uninit(void)
{
#if IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS)
#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL) || defined(CONFIG_MPSL_USE_ZEPHYR_PM)
	int err;
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL || CONFIG_MPSL_USE_ZEPHYR_PM */

#if defined(CONFIG_MPSL_CALIBRATION_PERIOD)
	atomic_set(&do_calibration, 0);
#endif /* CONFIG_MPSL_CALIBRATION_PERIOD */

	mpsl_lib_irq_disable();

	mpsl_uninit();

#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
	err = mpsl_clock_ctrl_uninit();
	if (err) {
		return err;
	}
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if defined(CONFIG_MPSL_USE_ZEPHYR_PM)
	err = mpsl_pm_utils_uninit();
	if (err) {
		return err;
	}
#endif /* CONFIG_MPSL_USE_ZEPHYR_PM */

	return 0;
#else /* !IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
	return -NRF_EPERM;
#endif /* IS_ENABLED(CONFIG_MPSL_DYNAMIC_INTERRUPTS) */
}

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
void mpsl_constlat_request_callback(void)
{
#if defined(CONFIG_NRFX_POWER)
	nrfx_power_constlat_mode_request();
#else
	nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
#endif
}

void mpsl_lowpower_request_callback(void)
{
#if defined(CONFIG_NRFX_POWER)
	nrfx_power_constlat_mode_free();
#else
	nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_LOWPWR);
#endif
}
#endif /* defined(CONFIG_SOC_COMPATIBLE_NRF54LX) */

#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL)
#define MPSL_INIT_LEVEL POST_KERNEL
#else
#define MPSL_INIT_LEVEL PRE_KERNEL_1
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL */

#if defined(CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL) && \
	defined(CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO)
BUILD_ASSERT(CONFIG_MPSL_INIT_PRIORITY > CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO,
			 "MPSL must be initialized after NRFS IPC");
#endif /* CONFIG_MPSL_USE_EXTERNAL_CLOCK_CONTROL && CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO */

SYS_INIT(mpsl_lib_init_sys, MPSL_INIT_LEVEL, CONFIG_MPSL_INIT_PRIORITY);
SYS_INIT(mpsl_low_prio_init, POST_KERNEL, CONFIG_MPSL_INIT_PRIORITY);
