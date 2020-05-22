/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 *   This file contains standalone implementation of the nRF 802.15.4 timer abstraction.
 *
 * This implementation is built on top of the RTC peripheral.
 *
 */

#include <platform/lp_timer/nrf_802154_lp_timer.h>

#include <assert.h>

#include <nrf.h>
#include <hal/nrf_rtc.h>

#include <platform/clock/nrf_802154_clock.h>
#include <platform/irq/nrf_802154_irq.h>
#include <nrf_802154_sl_config.h>
#include <nrf_802154_sl_periphs.h>
#include <nrf_802154_sl_utils.h>

#define RTC_LP_TIMER_COMPARE_CHANNEL 0
#define RTC_LP_TIMER_COMPARE_INT_MASK NRF_RTC_INT_COMPARE0_MASK
#define RTC_LP_TIMER_COMPARE_EVENT NRF_RTC_EVENT_COMPARE_0
#define RTC_LP_TIMER_COMPARE_EVENT_MASK RTC_EVTEN_COMPARE0_Msk

#define RTC_SYNC_COMPARE_CHANNEL 1
#define RTC_SYNC_COMPARE_INT_MASK NRF_RTC_INT_COMPARE1_MASK
#define RTC_SYNC_COMPARE_EVENT NRF_RTC_EVENT_COMPARE_1
#define RTC_SYNC_COMPARE_EVENT_MASK RTC_EVTEN_COMPARE1_Msk

/* Time that has passed between overflow events. On full RTC speed, it occurs every 512 s. */
#define US_PER_OVERFLOW (512UL * NRF_802154_SL_US_PER_S)
/* Minimum time delta from now before RTC compare event is guaranteed to fire. */
#define MIN_RTC_COMPARE_EVENT_DT (2 * NRF_802154_SL_US_PER_TICK)

#define EPOCH_32BIT_US (1ULL << 32)
#define EPOCH_FROM_TIME(time) ((time) & ((uint64_t)UINT32_MAX << 32))

#define MAX_LP_TIMER_SYNC_ITERS 4

/* Struct holding information about compare channel. */
typedef struct {
	/* Channel number */
	uint32_t channel;
	/* Interrupt mask */
	uint32_t int_mask;
	/* Event */
	nrf_rtc_event_t event;
	/* Event mask */
	uint32_t event_mask;
} compare_channel_descriptor_t;

/* Enum holding all used compare channels. */
typedef enum { LP_TIMER_CHANNEL, SYNC_CHANNEL, CHANNEL_CNT } compare_channel_t;

/* Descriptors of all used compare channels. */
static const compare_channel_descriptor_t cmp_ch[CHANNEL_CNT] = {
	{ RTC_LP_TIMER_COMPARE_CHANNEL, RTC_LP_TIMER_COMPARE_INT_MASK,
	  RTC_LP_TIMER_COMPARE_EVENT, RTC_LP_TIMER_COMPARE_EVENT_MASK },
	{ RTC_SYNC_COMPARE_CHANNEL, RTC_SYNC_COMPARE_INT_MASK,
	  RTC_SYNC_COMPARE_EVENT, RTC_SYNC_COMPARE_EVENT_MASK }
};

/* Target time of given channel [us]. */
static uint64_t target_times[CHANNEL_CNT];

/* Information that RTC interrupt was enabled while entering critical section. */
static volatile uint32_t lp_timer_irq_enabled;
/* Counter of RTC overflows, incremented by 2 on each OVERFLOW event. */
static volatile uint32_t offset_counter;
/* Mutex for write access to @ref offset_counter. */
static volatile uint8_t mutex;
/* Information that LFCLK is ready. */
static volatile bool clock_ready;
/* Information if timer should fire immediately. */
static volatile bool shall_fire_immediately;

/** @brief Forward declaration of RTC interrupt handler. */
static void rtc_irq_handler(void);

static uint32_t overflow_counter_get(void);

/** @brief Non-blocking mutex for mutual write access to @ref offset_counter variable.
 *
 *  @retval  true   Mutex was acquired.
 *  @retval  false  Mutex could not be acquired.
 */
static inline bool mutex_get(void)
{
	do {
		volatile uint8_t mutex_value = __LDREXB(&mutex);

		if (mutex_value) {
			__CLREX();
			return false;
		}
	} while (__STREXB(1, &mutex));

	/* Disable OVERFLOW interrupt to prevent lock-up in interrupt context while mutex is locked from lower priority context
	 * and OVERFLOW event flag is stil up. */
	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

	__DMB();

	return true;
}

/** @brief Release mutex. */
static inline void mutex_release(void)
{
	/* Re-enable OVERFLOW interrupt. */
	nrf_rtc_int_enable(NRF_802154_RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

	__DMB();
	mutex = 0;
}

/** @brief Check if timer shall strike.
 *
 *  @param[in]  now  Current time.
 *
 *  @retval  true   Timer shall strike now.
 *  @retval  false  Timer shall not strike now.
 */
static inline bool shall_strike(uint64_t now)
{
	return now >= target_times[LP_TIMER_CHANNEL];
}

/** @brief Convert time in [us] to RTC ticks.
 *
 *  @param[in]  time  Time to convert.
 *
 *  @return  Time value in RTC ticks.
 */
static inline uint64_t time_to_ticks(uint64_t time)
{
	return NRF_802154_SL_US_TO_RTC_TICKS(time);
}

/** @brief Convert RTC ticks to time in [us].
 *
 *  @param[in]  ticks  RTC ticks to convert.
 *
 *  @return  Time value in [us].
 */
static inline uint64_t ticks_to_time(uint64_t ticks)
{
	return NRF_802154_SL_RTC_TICKS_TO_US(ticks);
}

/** @brief Get current value of the RTC counter.
 *
 * @return  RTC counter value [ticks].
 */
static uint32_t counter_get(void)
{
	return nrf_rtc_counter_get(NRF_802154_RTC_INSTANCE);
}

/** @brief Get RTC counter value and matching offset that represent the current time.
 *
 * @param[out] p_offset   Offset of the current time.
 * @param[out] p_counter  RTC value of the current time.
 */
static void offset_and_counter_get(uint32_t *p_offset, uint32_t *p_counter)
{
	uint32_t offset_1 = overflow_counter_get();

	__DMB();

	uint32_t rtc_value_1 = counter_get();

	__DMB();

	uint32_t offset_2 = overflow_counter_get();

	*p_offset = offset_2;
	*p_counter = (offset_1 == offset_2) ? rtc_value_1 : counter_get();
}

/** @brief Get time from given @p offset and @p counter values.
 *
 *  @param[in] offset   Offset of time to get.
 *  @param[in] counter  RTC value representing time to get.
 *
 *  @return  Time calculated from given offset and counter [us].
 */
static uint64_t time_get(uint32_t offset, uint32_t counter)
{
	return (uint64_t)offset * US_PER_OVERFLOW + ticks_to_time(counter);
}

/** @brief Get current time.
 *
 *  @return  Current time in [us].
 */
static uint64_t curr_time_get(void)
{
	uint32_t offset;
	uint32_t rtc_value;

	offset_and_counter_get(&offset, &rtc_value);

	return time_get(offset, rtc_value);
}

/** @brief Get current overflow counter and handle OVERFLOW event if present.
 *
 *  This function returns current value of offset_counter variable. If OVERFLOW event is present
 *  while calling this function, it is handled within it.
 *
 *  @return  Current number of OVERFLOW events since platform start.
 */
static uint32_t overflow_counter_get(void)
{
	uint32_t offset;

	/* Get mutual access for writing to offset_counter variable. */
	if (mutex_get()) {
		bool increasing = false;

		/* Check if interrupt was handled already. */
		if (nrf_rtc_event_check(NRF_802154_RTC_INSTANCE,
					NRF_RTC_EVENT_OVERFLOW)) {
			offset_counter++;
			increasing = true;

			__DMB();

			/* Mark that interrupt was handled. */
			nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
					    NRF_RTC_EVENT_OVERFLOW);

			/* Result should be incremented. offset_counter will be incremented after mutex is released. */
		} else {
			/* Either overflow handling is not needed OR we acquired the mutex just after it was released.
			 * Overflow is handled after mutex is released, but it cannot be assured that offset_counter
			 * was incremented for the second time, so we increment the result here. */
		}

		offset = (offset_counter + 1) / 2;

		mutex_release();

		if (increasing) {
			/* It's virtually impossible that overflow event is pending again before next instruction is performed. It is an error condition. */
			assert(offset_counter & 0x01);

			/* Increment the counter for the second time, to alloww instructions from other context get correct value of the counter. */
			offset_counter++;
		}
	} else {
		/* Failed to acquire mutex. */
		if (nrf_rtc_event_check(NRF_802154_RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW) ||
			(offset_counter & 0x01)) {
			/* Lower priority context is currently incrementing offset_counter variable. */
			offset = (offset_counter + 2) / 2;
		} else {
			/* Lower priority context has already incremented offset_counter variable or incrementing is not needed now. */
			offset = offset_counter / 2;
		}
	}

	return offset;
}

/** @brief Handle COMPARE event. */
static void handle_compare_match(bool skip_check)
{
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].event);

	/* In case the target time was larger than single overflow,
	 * we should only strike the timer on final compare event. */
	if (skip_check || shall_strike(curr_time_get())) {
		nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE,
				      cmp_ch[LP_TIMER_CHANNEL].event_mask);
		nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
				    cmp_ch[LP_TIMER_CHANNEL].int_mask);

		nrf_802154_lp_timer_fired();
	}
}

/**
 * @brief Convert t0 and dt to 64 bit time.
 *
 * @note This function takes into account possible overflow of first 32 bits in current time.
 *
 * @return  Converted time in [us].
 */
static uint64_t convert_to_64bit_time(uint32_t t0, uint32_t dt,
				      const uint64_t *p_now)
{
	uint64_t now;

	now = *p_now;

	/* Check if 32 LSB of `now` overflowed between getting t0 and loading `now` value. */
	if (((uint32_t)now < t0) && ((t0 - (uint32_t)now) > (UINT32_MAX / 2))) {
		now -= EPOCH_32BIT_US;
	} else if (((uint32_t)now > t0) &&
		   (((uint32_t)now) - t0 > (UINT32_MAX / 2))) {
		now += EPOCH_32BIT_US;
	}

	return (EPOCH_FROM_TIME(now)) + t0 + dt;
}

/**
 * @brief Round time up to multiple of the timer ticks.
 */
static uint64_t round_up_to_timer_ticks_multiply(uint64_t time)
{
	uint64_t ticks = time_to_ticks(time);
	uint64_t result = ticks_to_time(ticks);

	return result;
}

/**
 * @brief Start one-shot timer that expires at specified time on desired channel.
 *
 * Start one-shot timer that will expire @p dt microseconds after @p t0 time on channel @p channel.
 *
 * @param[in]  channel  Compare channel on which timer will be started.
 * @param[in]  t0       Number of microseconds representing timer start time.
 * @param[in]  dt       Time of timer expiration as time elapsed from @p t0 [us].
 * @param[in]  p_now    Pointer to data with the current time.
 */
static void timer_start_at(compare_channel_t channel, uint32_t t0, uint32_t dt,
			   const uint64_t *p_now)
{
	uint64_t target_counter;
	uint64_t target_time;

	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
			    cmp_ch[channel].int_mask);
	nrf_rtc_event_enable(NRF_802154_RTC_INSTANCE,
			     cmp_ch[channel].event_mask);

	target_time = convert_to_64bit_time(t0, dt, p_now);
	target_counter = time_to_ticks(target_time);

	target_times[channel] = round_up_to_timer_ticks_multiply(target_time);

	nrf_rtc_cc_set(NRF_802154_RTC_INSTANCE, cmp_ch[channel].channel,
		       target_counter);
}

/**
 * @brief Start synchronization timer at given time.
 *
 * @param[in]  t0       Number of microseconds representing timer start time.
 * @param[in]  dt       Time of timer expiration as time elapsed from @p t0 [us].
 * @param[in]  p_now    Pointer to data with current time.
 */
static void timer_sync_start_at(uint32_t t0, uint32_t dt, const uint64_t *p_now)
{
	timer_start_at(SYNC_CHANNEL, t0, dt, p_now);

	nrf_rtc_int_enable(NRF_802154_RTC_INSTANCE,
			   cmp_ch[SYNC_CHANNEL].int_mask);
}

void nrf_802154_lp_timer_init(void)
{
	offset_counter = 0;
	target_times[LP_TIMER_CHANNEL] = 0;
	clock_ready = false;
	lp_timer_irq_enabled = 0;

	/* Setup low frequency clock. */
	nrf_802154_clock_lfclk_start();

	while (!clock_ready) {
		/* Intentionally empty */
	}

	/* Setup RTC timer. */
#if !NRF_802154_IRQ_PRIORITY_ALLOWED(NRF_802154_SL_RTC_IRQ_PRIORITY)
#error NRF_802154_SL_RTC_IRQ_PRIORITY value out of the allowed range.
#endif
	nrf_802154_irq_init(NRF_802154_RTC_IRQN, NRF_802154_SL_RTC_IRQ_PRIORITY,
			    rtc_irq_handler);
	nrf_802154_irq_enable(NRF_802154_RTC_IRQN);

	nrf_rtc_prescaler_set(NRF_802154_RTC_INSTANCE, 0);

	/* Setup RTC events. */
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);
	nrf_rtc_event_enable(NRF_802154_RTC_INSTANCE, RTC_EVTEN_OVRFLW_Msk);
	nrf_rtc_int_enable(NRF_802154_RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].int_mask);
	nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE,
			      cmp_ch[LP_TIMER_CHANNEL].event_mask);
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].event);

	/* Start RTC timer. */
	nrf_rtc_task_trigger(NRF_802154_RTC_INSTANCE, NRF_RTC_TASK_START);
}

void nrf_802154_lp_timer_deinit(void)
{
	nrf_rtc_task_trigger(NRF_802154_RTC_INSTANCE, NRF_RTC_TASK_STOP);

	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].int_mask);
	nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE,
			      cmp_ch[LP_TIMER_CHANNEL].event_mask);
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].event);

	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);
	nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE, RTC_EVTEN_OVRFLW_Msk);
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);

	nrf_802154_lp_timer_sync_stop();

	nrf_802154_irq_disable(NRF_802154_RTC_IRQN);
	nrf_802154_irq_clear_pending(NRF_802154_RTC_IRQN);

	nrf_802154_clock_lfclk_stop();
}

void nrf_802154_lp_timer_critical_section_enter(void)
{
	if (nrf_802154_irq_is_enabled(NRF_802154_RTC_IRQN)) {
		lp_timer_irq_enabled = 1;
	}

	nrf_802154_irq_disable(NRF_802154_RTC_IRQN);
}

void nrf_802154_lp_timer_critical_section_exit(void)
{
	if (lp_timer_irq_enabled) {
		lp_timer_irq_enabled = 0;
		nrf_802154_irq_enable(NRF_802154_RTC_IRQN);
	}
}

uint32_t nrf_802154_lp_timer_time_get(void)
{
	return (uint32_t)curr_time_get();
}

uint32_t nrf_802154_lp_timer_granularity_get(void)
{
	return NRF_802154_SL_US_PER_TICK;
}

void nrf_802154_lp_timer_start(uint32_t t0, uint32_t dt)
{
	uint32_t offset;
	uint32_t rtc_value;
	uint64_t now;

	offset_and_counter_get(&offset, &rtc_value);
	now = time_get(offset, rtc_value);

	timer_start_at(LP_TIMER_CHANNEL, t0, dt, &now);

	if (rtc_value != counter_get()) {
		now = curr_time_get();
	}

	if (shall_strike(now + MIN_RTC_COMPARE_EVENT_DT)) {
		shall_fire_immediately = true;
		nrf_802154_irq_set_pending(NRF_802154_RTC_IRQN);
	} else {
		nrf_rtc_int_enable(NRF_802154_RTC_INSTANCE,
				   cmp_ch[LP_TIMER_CHANNEL].int_mask);
	}
}

bool nrf_802154_lp_timer_is_running(void)
{
	return nrf_rtc_int_enable_check(NRF_802154_RTC_INSTANCE,
					cmp_ch[LP_TIMER_CHANNEL].int_mask);
}

void nrf_802154_lp_timer_stop(void)
{
	nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE,
			      cmp_ch[LP_TIMER_CHANNEL].event_mask);
	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].int_mask);
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
			    cmp_ch[LP_TIMER_CHANNEL].event);
}

void nrf_802154_lp_timer_sync_start_now(void)
{
	uint32_t counter;
	uint32_t offset;
	uint64_t now;
	uint32_t iterations = MAX_LP_TIMER_SYNC_ITERS;

	do {
		offset_and_counter_get(&offset, &counter);
		now = time_get(offset, counter);
		timer_sync_start_at((uint32_t)now, MIN_RTC_COMPARE_EVENT_DT,
				    &now);
	} while ((counter_get() != counter) && (--iterations > 0));
}

void nrf_802154_lp_timer_sync_start_at(uint32_t t0, uint32_t dt)
{
	uint64_t now = curr_time_get();

	timer_sync_start_at(t0, dt, &now);
}

void nrf_802154_lp_timer_sync_stop(void)
{
	nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE,
			      cmp_ch[SYNC_CHANNEL].event_mask);
	nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
			    cmp_ch[SYNC_CHANNEL].int_mask);
	nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
			    cmp_ch[SYNC_CHANNEL].event);
}

uint32_t nrf_802154_lp_timer_sync_event_get(void)
{
	return nrf_rtc_event_address_get(NRF_802154_RTC_INSTANCE,
					 cmp_ch[SYNC_CHANNEL].event);
}

uint32_t nrf_802154_lp_timer_sync_time_get(void)
{
	return (uint32_t)target_times[SYNC_CHANNEL];
}

void nrf_802154_clock_lfclk_ready(void)
{
	clock_ready = true;
}

static void rtc_irq_handler(void)
{
	/* Handle overflow. */
	if (nrf_rtc_event_check(NRF_802154_RTC_INSTANCE,
				NRF_RTC_EVENT_OVERFLOW)) {
		/* Disable OVERFLOW interrupt to prevent lock-up in interrupt context while mutex is locked from lower priority context
		 * and OVERFLOW event flag is stil up.
		 * OVERFLOW interrupt will be re-enabled when mutex is released - either from this handler, or from lower priority context,
		 * that locked the mutex. */
		nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
				    NRF_RTC_INT_OVERFLOW_MASK);

		/* Handle OVERFLOW event by reading current value of overflow counter. */
		(void)overflow_counter_get();
	}

	/* Handle compare match. */
	if (shall_fire_immediately) {
		shall_fire_immediately = false;
		handle_compare_match(true);
	}

	if (nrf_rtc_int_enable_check(NRF_802154_RTC_INSTANCE,
				     cmp_ch[LP_TIMER_CHANNEL].int_mask) &&
	    nrf_rtc_event_check(NRF_802154_RTC_INSTANCE,
				cmp_ch[LP_TIMER_CHANNEL].event)) {
		handle_compare_match(false);
	}

	if (nrf_rtc_int_enable_check(NRF_802154_RTC_INSTANCE,
				     cmp_ch[SYNC_CHANNEL].int_mask) &&
	    nrf_rtc_event_check(NRF_802154_RTC_INSTANCE,
				cmp_ch[SYNC_CHANNEL].event)) {
		nrf_rtc_event_clear(NRF_802154_RTC_INSTANCE,
				    cmp_ch[SYNC_CHANNEL].event);
		nrf_rtc_event_disable(NRF_802154_RTC_INSTANCE,
				      cmp_ch[SYNC_CHANNEL].event_mask);
		nrf_rtc_int_disable(NRF_802154_RTC_INSTANCE,
				    cmp_ch[SYNC_CHANNEL].int_mask);
		nrf_802154_lp_timer_synchronized();
	}
}

void NRF_802154_RTC_IRQ_HANDLER(void)
{
	rtc_irq_handler();
}

#ifndef UNITY_ON_TARGET
__WEAK void nrf_802154_lp_timer_synchronized(void)
{
	/* Intentionally empty */
}

#endif /* UNITY_ON_TARGET */
