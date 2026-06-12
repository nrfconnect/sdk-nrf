/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * RT component (fast path) of the app-domain RT framework.
 *
 * This translation unit owns the RT resources, all selected from devicetree
 * (see rt_platform.h):
 *  - rt-timer alias     : periodic time base, driven via the nrfx HAL.
 *  - rt-gpios           : toggled by the zero-latency ISR.
 *  - rt-debug-gpios     : optional ISR-duration debug pin.
 *  - rt-egu alias       : optional RT -> Zephyr signaling (data plane).
 *  - rt_ctrl / rt_evq   : statically allocated shared RAM.
 *
 * The ISR runs as a Zero-Latency Interrupt, i.e. outside Zephyr scheduling and
 * outside the reach of irq_lock(). It must therefore never call Zephyr kernel
 * APIs, logging, or dynamic allocation, and must stay as short as possible.
 * On SoCs where MPSL runs on the same core (e.g. nRF54L), the ZLI shares the
 * highest NVIC priority with the MPSL radio ISRs, which makes a short ISR even
 * more important.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <cmsis_core.h>

#include <hal/nrf_timer.h>
#include <hal/nrf_gpio.h>
#if defined(CONFIG_RT_FW_DATA_PLANE)
#include <hal/nrf_egu.h>
#endif

#include "rt_comm.h"
#include "rt_shared.h"
#include "rt_platform.h"

/* Target RT time-base rate: 1 MHz -> 1 tick == 1 us, derived per SoC from the
 * timer base frequency (so the prescaler is not hard-coded to one clock).
 */
#define RT_TIMER_TARGET_HZ 1000000U

/* Minimum compare value to keep the periodic interrupt sane. */
#define RT_MIN_PERIOD_TICKS 2U

/* Effective RT time-base configuration, computed once from the SoC clock. */
static uint8_t rt_prescaler;
static uint32_t rt_timer_hz;

static void rt_ensure_clock(void)
{
	uint32_t base_hz;
	uint8_t presc = 0U;

	if (rt_timer_hz != 0U) {
		return;
	}

	base_hz = NRF_TIMER_BASE_FREQUENCY_GET(RT_TIMER);
	while (((base_hz >> presc) > RT_TIMER_TARGET_HZ) && (presc < 9U)) {
		presc++;
	}

	rt_prescaler = presc;
	rt_timer_hz = base_hz >> presc;
}

/* Shared RAM, owned by the RT component (see rt_comm.h). */
struct rt_control_block rt_ctrl;
#if defined(CONFIG_RT_FW_DATA_PLANE)
struct rt_event_queue rt_evq __aligned(RT_SHARED_ALIGN);

#if defined(CONFIG_RT_FW_SHARED_DMA_VISIBLE)
/* When the event payload is treated as DMA-/cross-master-visible, each entry
 * must own whole cache line(s) so a per-entry flush/invalidate cannot clobber a
 * neighbouring entry or the head/tail indices.
 */
BUILD_ASSERT((sizeof(struct rt_event) % RT_SHARED_ALIGN) == 0U,
	     "rt_event size must be a multiple of the cache line size");
BUILD_ASSERT(__alignof__(struct rt_event) >= RT_SHARED_ALIGN,
	     "rt_event must be cache-line aligned");
BUILD_ASSERT(offsetof(struct rt_event_queue, entries) >= RT_SHARED_ALIGN,
	     "event entries must not share a cache line with head/tail");
#endif
#endif

#if defined(CONFIG_RT_FW_DATA_PLANE)
/* Single-producer push from the RT ISR: drops the event if the ring is full. */
static inline void rt_push_event(uint32_t type, uint32_t value)
{
	uint32_t head = rt_evq.head;
	uint32_t next = (head + 1U) & (RT_EVENT_QUEUE_SIZE - 1U);

	if (next == rt_evq.tail) {
		/* Ring full: drop and count it (saturating) so the consumer can
		 * see the scale of the overflow, not just that one happened.
		 */
		if (rt_ctrl.dropped_events != UINT32_MAX) {
			rt_ctrl.dropped_events++;
		}
		return;
	}

	rt_evq.entries[head].type = type;
	rt_evq.entries[head].value = value;
	rt_evq.entries[head].timestamp = rt_ctrl.tick_count;

	/* The entry is a unidirectional payload (RT writes, Zephyr reads), so it
	 * is safe to flush it towards a DMA/other-master observer when that
	 * opt-in is enabled. On the default same-core path this is just the
	 * release barrier.
	 */
	rt_shared_dma_flush(&rt_evq.entries[head], sizeof(rt_evq.entries[head]));
	rt_shared_release();
	rt_evq.head = next;

	nrf_egu_task_trigger(RT_EGU, NRF_EGU_TASK_TRIGGER0);
}
#endif /* CONFIG_RT_FW_DATA_PLANE */

/*
 * Consume a pending control-plane command at a tick boundary using the seqlock
 * protocol (see rt_comm.h). Reads a consistent {enable, period_ticks} snapshot,
 * programs the hardware and publishes the applied state. Returns true when a
 * command was applied (so the caller acks exactly that command_seq).
 *
 * If the writer is mid-update (odd command_seq) or the snapshot changes under
 * us, nothing is applied and the command stays pending for the next tick. The
 * ISR therefore never spins.
 */
static inline bool rt_consume_command(void)
{
	uint32_t seq = rt_ctrl.command_seq;
	uint32_t enable;
	uint32_t ticks;

	/* Odd => writer in progress; nothing new => already up to date. */
	if ((seq & 1U) != 0U || seq == rt_ctrl.ack_seq) {
		return false;
	}

	/* Acquire: observing the even command_seq happens-before reading the
	 * command payload it published.
	 */
	rt_shared_acquire();
	enable = rt_ctrl.enable;
	ticks = rt_ctrl.period_ticks;
	rt_shared_acquire();

	/* The snapshot is only valid if the writer did not touch the command
	 * between the two reads of command_seq.
	 */
	if (rt_ctrl.command_seq != seq) {
		return false;
	}

	if (ticks < RT_MIN_PERIOD_TICKS) {
		ticks = RT_MIN_PERIOD_TICKS;
	}

	/* New compare value takes effect on the next compare cycle. */
	nrf_timer_cc_set(RT_TIMER, NRF_TIMER_CC_CHANNEL0, ticks);

	if (enable == 0U) {
		/* Drive the output to a defined idle level when disabled. */
		nrf_gpio_pin_clear(RT_GPIO_ABS);
	}

	/* Publish the state actually in effect, then ack this exact command. */
	rt_ctrl.applied_period_ticks = ticks;
	rt_ctrl.applied_enable = enable;
	rt_shared_release();
	rt_ctrl.ack_seq = seq;

#if defined(CONFIG_RT_FW_DATA_PLANE)
	rt_push_event(RT_EVENT_CONFIG_APPLIED, ticks);
#endif
	return true;
}

/*
 * Zero-latency timer ISR. The TIMER COMPARE0 short auto-clears the counter,
 * so this fires periodically. The body is intentionally tiny.
 */
ISR_DIRECT_DECLARE(rt_fastpath_isr)
{
	if (nrf_timer_event_check(RT_TIMER, NRF_TIMER_EVENT_COMPARE0)) {
		nrf_timer_event_clear(RT_TIMER, NRF_TIMER_EVENT_COMPARE0);

#if defined(CONFIG_RT_FW_DEBUG_PIN)
		nrf_gpio_pin_set(RT_DEBUG_ABS);
#endif

		/* Control plane: apply at most one pending command per tick. */
		(void)rt_consume_command();

		/* Real-time work, gated by the state actually applied (not the
		 * requested one, which may still be mid-publish).
		 */
		if (rt_ctrl.applied_enable) {
			nrf_gpio_pin_toggle(RT_GPIO_ABS);
			rt_ctrl.tick_count++;
#if defined(CONFIG_RT_FW_DATA_PLANE)
			rt_push_event(RT_EVENT_TICK, rt_ctrl.tick_count);
#endif
		}

#if defined(CONFIG_RT_FW_ISR_TEST_LOAD)
		for (volatile uint32_t i = 0; i < CONFIG_RT_FW_ISR_TEST_LOAD_CYCLES; i++) {
			__NOP();
		}
#endif

#if defined(CONFIG_RT_FW_DEBUG_PIN)
		nrf_gpio_pin_clear(RT_DEBUG_ABS);
#endif
	}

	/* ZLI handlers never request a reschedule. */
	return 0;
}

uint32_t rt_fastpath_us_to_ticks(uint32_t us)
{
	uint64_t ticks;

	rt_ensure_clock();
	ticks = ((uint64_t)us * rt_timer_hz) / 1000000ULL;

	if (ticks < RT_MIN_PERIOD_TICKS) {
		ticks = RT_MIN_PERIOD_TICKS;
	}
	if (ticks > UINT32_MAX) {
		ticks = UINT32_MAX;
	}

	return (uint32_t)ticks;
}

uint32_t rt_fastpath_ticks_to_us(uint32_t ticks)
{
	uint64_t us;

	rt_ensure_clock();
	us = ((uint64_t)ticks * 1000000ULL) / rt_timer_hz;

	if (us > UINT32_MAX) {
		us = UINT32_MAX;
	}

	return (uint32_t)us;
}

void rt_fastpath_init(uint32_t initial_period_ticks)
{
	/* RT-owned GPIO outputs. */
	nrf_gpio_cfg_output(RT_GPIO_ABS);
	nrf_gpio_pin_clear(RT_GPIO_ABS);
#if defined(CONFIG_RT_FW_DEBUG_PIN)
	nrf_gpio_cfg_output(RT_DEBUG_ABS);
	nrf_gpio_pin_clear(RT_DEBUG_ABS);
#endif

	/* The applied state mirrors the hardware programmed below: the timer
	 * runs at initial_period_ticks but output toggling is off until enabled.
	 */
	rt_ctrl.applied_enable = 0U;
	rt_ctrl.applied_period_ticks = initial_period_ticks;

	/* RT-owned time base, configured directly through the HAL. */
	rt_ensure_clock();
	nrf_timer_task_trigger(RT_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(RT_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_mode_set(RT_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(RT_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_prescaler_set(RT_TIMER, rt_prescaler);
	nrf_timer_cc_set(RT_TIMER, NRF_TIMER_CC_CHANNEL0, initial_period_ticks);
	nrf_timer_shorts_enable(RT_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_event_clear(RT_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(RT_TIMER, NRF_TIMER_INT_COMPARE0_MASK);

	/* Install the timer interrupt as a Zero-Latency IRQ. The priority
	 * argument is overridden by Zephyr to the ZLI level.
	 */
	IRQ_DIRECT_CONNECT(RT_TIMER_IRQN, 0, rt_fastpath_isr, IRQ_ZERO_LATENCY);
	irq_enable(RT_TIMER_IRQN);

	/* The RT execution context is always alive after init. The timer runs
	 * and the ISR polls the control plane every tick. GPIO toggling itself
	 * is gated by rt_ctrl.enable.
	 */
	nrf_timer_task_trigger(RT_TIMER, NRF_TIMER_TASK_START);
}
