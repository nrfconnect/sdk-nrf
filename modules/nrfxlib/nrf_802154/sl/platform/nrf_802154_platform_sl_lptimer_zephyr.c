/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "platform/nrf_802154_platform_sl_lptimer.h"

#include <assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/timer/nrf_rtc_timer.h>
#include <zephyr/sys/barrier.h>
#include <hal/nrf_rtc.h>
#include <helpers/nrfx_gppi.h>

#include "platform/nrf_802154_clock.h"
#include "nrf_802154_sl_utils.h"

#define RTC_CHAN_INVALID (-1)
#if defined(CONFIG_SOC_NRF5340_CPUNET)

BUILD_ASSERT(CONFIG_MPSL);

#define HW_TASK_RTC	     NRF_RTC0
#define HW_TASK_RTC_CHAN (3)

#define RTC_COUNTER_BIT_WIDTH 24U
#define RTC_COUNTER_SPAN      BIT(RTC_COUNTER_BIT_WIDTH)
#define RTC_COUNTER_MAX       (RTC_COUNTER_SPAN - 1U)
#define RTC_COUNTER_HALF_SPAN (RTC_COUNTER_SPAN / 2U)

#define RTC_MIN_CYCLES_FROM_NOW 3
#endif

struct timer_desc {
	z_nrf_rtc_timer_compare_handler_t handler;
	uint64_t target_time;
	int32_t chan;
	int32_t int_lock_key;
};

enum hw_task_state_type {
	HW_TASK_STATE_IDLE,
	HW_TASK_STATE_SETTING_UP,
	HW_TASK_STATE_READY,
	HW_TASK_STATE_UPDATING,
	HW_TASK_STATE_CLEANING
};

struct hw_task_desc {
	atomic_t state;
	int32_t chan;
	int32_t ppi;
	uint64_t fire_lpticks;
};

static struct timer_desc m_timer;
static struct timer_desc m_sync_timer;
static struct hw_task_desc m_hw_task;
static volatile bool m_clock_ready;
static bool m_in_critical_section;

void timer_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	(void)user_data;
	(void)expire_time;

	assert(id == m_timer.chan);

	uint64_t curr_time = z_nrf_rtc_timer_read();

	nrf_802154_sl_timer_handler(curr_time);
}

/**
 * @brief RTC IRQ handler for synchronization timer channel.
 */
void sync_timer_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	(void)user_data;

	assert(id == m_sync_timer.chan);

	/**
	 * Expire time might have been different than the desired target
	 * time. Update it so that a more accurate value can be returned
	 * by nrf_802154_lp_timer_sync_time_get.
	 */
	m_sync_timer.target_time = expire_time;

	nrf_802154_sl_timestamper_synchronized();
}

/**
 * @brief Start one-shot timer that expires at specified time on desired channel.
 *
 * Start one-shot timer that will expire when @p target_time is reached on channel @p channel.
 *
 * @param[inout] timer        Pointer to descriptor of timer to set.
 * @param[in]    target_time  Absolute target time in microseconds.
 */
static void timer_start_at(struct timer_desc *timer, uint64_t target_time)
{
	nrf_802154_sl_mcu_critical_state_t state;

	z_nrf_rtc_timer_set(timer->chan, target_time, timer->handler, NULL);

	nrf_802154_sl_mcu_critical_enter(state);

	timer->target_time = target_time;

	nrf_802154_sl_mcu_critical_exit(state);
}

#if defined(CONFIG_SOC_NRF5340_CPUNET)
static inline uint32_t rtc_counter_diff(uint32_t a, uint32_t b)
{
	return (a - b) & RTC_COUNTER_MAX;
}
#endif

static bool hw_task_state_set(enum hw_task_state_type expected_state,
			      enum hw_task_state_type new_state)
{
	return atomic_cas(&m_hw_task.state, expected_state, new_state);
}

static inline uint32_t hw_task_rtc_get_compare_evt_address(int32_t cc_num)
{
#if defined(CONFIG_SOC_NRF5340_CPUNET)
	return nrf_rtc_event_address_get(HW_TASK_RTC, nrf_rtc_compare_event_get(cc_num));
#else
	return z_nrf_rtc_timer_compare_evt_address_get(cc_num);
#endif
}

static void hw_task_rtc_cc_bind_to_ppi(int32_t cc_num, uint32_t ppi_num)
{
	uint32_t event_address = hw_task_rtc_get_compare_evt_address(cc_num);

	nrfx_gppi_event_endpoint_setup(ppi_num, event_address);
}

static void hw_task_rtc_cc_unbind(int32_t cc_num, uint32_t ppi_num)
{
	if (ppi_num != NRF_802154_SL_HW_TASK_PPI_INVALID) {
		uint32_t event_address = hw_task_rtc_get_compare_evt_address(cc_num);

		nrfx_gppi_event_endpoint_clear(ppi_num, event_address);
	}
}

static bool hw_task_rtc_cc_event_check(int32_t cc_num)
{
	uint32_t event_address = hw_task_rtc_get_compare_evt_address(cc_num);

	return *(volatile uint32_t *)event_address;
}

static void hw_task_rtc_timer_abort(void)
{
#if defined(CONFIG_SOC_NRF5340_CPUNET)
	/* No interrupt disabling/clearing is needed, as HW task does not use interrupts*/
	nrf_rtc_event_disable(HW_TASK_RTC, NRF_RTC_CHANNEL_INT_MASK(m_hw_task.chan));
	nrf_rtc_event_clear(HW_TASK_RTC, NRF_RTC_CHANNEL_EVENT_ADDR(m_hw_task.chan));
#else
	z_nrf_rtc_timer_abort(m_hw_task.chan);
#endif
}

#if defined(CONFIG_SOC_NRF5340_CPUNET)
static uint32_t zephyr_rtc_ticks_to_hw_task_rtc_counter(uint64_t ticks)
{
	uint64_t zephyr_rtc_counter_now = 0;
	uint32_t hw_task_rtc_counter_now = 0;
	uint32_t hw_task_rtc_counter_now_2 = 0;

	/**
	 * The RTC0 counter is read twice - once before and once after reading RTC1.
	 * If the two reads are different, the procedure is repeated.
	 * This is to make sure that no RTC tick occurred between reading RTC0 and
	 * RTC1 and so the calculated difference between the timers is correct.
	 */
	do {
		hw_task_rtc_counter_now = nrf_rtc_counter_get(HW_TASK_RTC);
		zephyr_rtc_counter_now = z_nrf_rtc_timer_read();
		barrier_dmem_fence_full();
		hw_task_rtc_counter_now_2 = nrf_rtc_counter_get(HW_TASK_RTC);

	} while (hw_task_rtc_counter_now != hw_task_rtc_counter_now_2);

	/* This function can only be called in close proximity of ticks */
	assert(ticks >= zephyr_rtc_counter_now
		|| zephyr_rtc_counter_now - ticks < RTC_COUNTER_HALF_SPAN);
	assert(ticks <= zephyr_rtc_counter_now
		|| ticks - zephyr_rtc_counter_now < RTC_COUNTER_HALF_SPAN);

	uint32_t diff = rtc_counter_diff(hw_task_rtc_counter_now,
		(uint32_t) zephyr_rtc_counter_now % RTC_COUNTER_SPAN);

	return (uint32_t) ((ticks + diff) % RTC_COUNTER_SPAN);
}

static int hw_task_rtc_counter_compare_set(uint32_t cc_value)
{
	uint32_t current_counter_value = nrf_rtc_counter_get(HW_TASK_RTC);

	/* As the HW tasks are setup only shortly before they are triggered,
	 * it is certain that the required fire time is not further away
	 * than one half of the RTC counter span.
	 * If it is it means that the trigger time is already in the past.
	 */
	if (rtc_counter_diff(cc_value, current_counter_value) > RTC_COUNTER_HALF_SPAN) {
		return -EINVAL;
	}

	nrf_rtc_event_disable(HW_TASK_RTC, NRF_RTC_CHANNEL_INT_MASK(m_hw_task.chan));
	nrf_rtc_event_clear(HW_TASK_RTC, NRF_RTC_CHANNEL_EVENT_ADDR(m_hw_task.chan));

	nrf_rtc_cc_set(HW_TASK_RTC, m_hw_task.chan, cc_value);

	nrf_rtc_event_enable(HW_TASK_RTC, NRF_RTC_CHANNEL_INT_MASK(m_hw_task.chan));

	current_counter_value = nrf_rtc_counter_get(HW_TASK_RTC);

	if (rtc_counter_diff(cc_value, current_counter_value + RTC_MIN_CYCLES_FROM_NOW)
		> (RTC_COUNTER_HALF_SPAN - RTC_MIN_CYCLES_FROM_NOW)) {
		nrf_rtc_event_disable(HW_TASK_RTC, NRF_RTC_CHANNEL_INT_MASK(m_hw_task.chan));
		nrf_rtc_event_clear(HW_TASK_RTC, NRF_RTC_CHANNEL_EVENT_ADDR(m_hw_task.chan));
		return -EINVAL;
	}

	return 0;
}
#endif

static int hw_task_rtc_timer_set(uint64_t fire_lpticks)
{
#if defined(CONFIG_SOC_NRF5340_CPUNET)
	uint32_t hw_task_rtc_counter_ticks = zephyr_rtc_ticks_to_hw_task_rtc_counter(fire_lpticks);

	return hw_task_rtc_counter_compare_set(hw_task_rtc_counter_ticks);
#else
	return z_nrf_rtc_timer_exact_set(m_hw_task.chan, fire_lpticks, NULL, NULL);
#endif
}

void nrf_802154_platform_sl_lp_timer_init(void)
{
	m_in_critical_section = false;
	m_hw_task.state = HW_TASK_STATE_IDLE;
#if defined(CONFIG_SOC_NRF5340_CPUNET)
	m_hw_task.chan = HW_TASK_RTC_CHAN;
#else
	m_hw_task.chan = z_nrf_rtc_timer_chan_alloc();
	if (m_hw_task.chan < 0) {
		assert(false);
		return;
	}
#endif
	m_hw_task.ppi = NRF_802154_SL_HW_TASK_PPI_INVALID;
	m_timer.handler = timer_handler;
	m_sync_timer.handler = sync_timer_handler;

	m_timer.chan = z_nrf_rtc_timer_chan_alloc();
	if (m_timer.chan < 0) {
		assert(false);
		return;
	}

	m_sync_timer.chan = z_nrf_rtc_timer_chan_alloc();
	if (m_sync_timer.chan < 0) {
		assert(false);
		return;
	}
}

void nrf_802154_platform_sl_lp_timer_deinit(void)
{
	(void)z_nrf_rtc_timer_compare_int_lock(m_timer.chan);

	nrf_802154_platform_sl_lptimer_sync_abort();

	z_nrf_rtc_timer_chan_free(m_timer.chan);
	z_nrf_rtc_timer_chan_free(m_sync_timer.chan);

	if (m_hw_task.chan != RTC_CHAN_INVALID) {
#if !defined(CONFIG_SOC_NRF5340_CPUNET)
		z_nrf_rtc_timer_chan_free(m_hw_task.chan);
#endif
		m_hw_task.chan = RTC_CHAN_INVALID;
	}
}

uint64_t nrf_802154_platform_sl_lptimer_current_lpticks_get(void)
{
	return z_nrf_rtc_timer_read();
}

uint64_t nrf_802154_platform_sl_lptimer_us_to_lpticks_convert(uint64_t us, bool round_up)
{
	return NRF_802154_SL_US_TO_RTC_TICKS(us, round_up);
}

uint64_t nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
	/* Calculations are performed on uint64_t as it is safe to assume
	 * overflow will not occur in any foreseeable future.
	 */
	return NRF_802154_SL_RTC_TICKS_TO_US(lpticks);
}

void nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
	/* This function is not required to be reentrant, hence no critical section. */
	timer_start_at(&m_timer, fire_lpticks);
}

void nrf_802154_platform_sl_lptimer_disable(void)
{
	z_nrf_rtc_timer_abort(m_timer.chan);
}

void nrf_802154_platform_sl_lptimer_critical_section_enter(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	if (!m_in_critical_section) {
		m_timer.int_lock_key = z_nrf_rtc_timer_compare_int_lock(m_timer.chan);
		m_sync_timer.int_lock_key = z_nrf_rtc_timer_compare_int_lock(m_sync_timer.chan);
		m_in_critical_section = true;
	}

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_platform_sl_lptimer_critical_section_exit(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	m_in_critical_section = false;

	z_nrf_rtc_timer_compare_int_unlock(m_timer.chan, m_timer.int_lock_key);
	z_nrf_rtc_timer_compare_int_unlock(m_sync_timer.chan, m_sync_timer.int_lock_key);

	nrf_802154_sl_mcu_critical_exit(state);
}

uint32_t nrf_802154_platform_sl_lptimer_granularity_get(void)
{
	return NRF_802154_SL_US_PER_TICK;
}

void nrf_802154_platform_sl_lptimer_sync_schedule_now(void)
{
	uint64_t now = z_nrf_rtc_timer_read();

	/**
	 * Despite this function's name, synchronization is not expected to be
	 * scheduled for the current tick. Add a safe 3-tick margin
	 */
	timer_start_at(&m_sync_timer, now + 3);
}

void nrf_802154_platform_sl_lptimer_sync_schedule_at(uint64_t fire_lpticks)
{
	timer_start_at(&m_sync_timer, fire_lpticks);
}

void nrf_802154_platform_sl_lptimer_sync_abort(void)
{
	z_nrf_rtc_timer_abort(m_sync_timer.chan);
}

uint32_t nrf_802154_platform_sl_lptimer_sync_event_get(void)
{
	return z_nrf_rtc_timer_compare_evt_address_get(m_sync_timer.chan);
}

uint64_t nrf_802154_platform_sl_lptimer_sync_lpticks_get(void)
{
	return m_sync_timer.target_time;
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_prepare(
	uint64_t fire_lpticks,
	uint32_t ppi_channel)
{
	uint32_t evt_address;
	nrf_802154_sl_mcu_critical_state_t mcu_cs_state;

	if (!hw_task_state_set(HW_TASK_STATE_IDLE, HW_TASK_STATE_SETTING_UP)) {
		/* The only one available set of peripherals is already used. */
		return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
	}

	if (hw_task_rtc_timer_set(fire_lpticks) != 0) {
		hw_task_state_set(HW_TASK_STATE_SETTING_UP, HW_TASK_STATE_IDLE);
		uint64_t now = z_nrf_rtc_timer_read();

		if ((fire_lpticks > now) &&
		    (fire_lpticks - now > NRF_RTC_TIMER_MAX_SCHEDULE_SPAN/2)) {
			return NRF_802154_SL_LPTIMER_PLATFORM_TOO_DISTANT;
		}
		return NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE;
	}

	evt_address = hw_task_rtc_get_compare_evt_address(m_hw_task.chan);

	nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

	/* For triggering to take place a safe margin is 2 lpticks from `now`. */
	if ((z_nrf_rtc_timer_read() + 2) > fire_lpticks) {
		/* it is too late */
		nrf_802154_sl_mcu_critical_exit(mcu_cs_state);
		hw_task_rtc_cc_unbind(m_hw_task.chan, ppi_channel);
		m_hw_task.ppi = NRF_802154_SL_HW_TASK_PPI_INVALID;
		hw_task_rtc_timer_abort();
		hw_task_state_set(HW_TASK_STATE_SETTING_UP, HW_TASK_STATE_IDLE);
		return NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE;
	}

	if (ppi_channel != NRF_802154_SL_HW_TASK_PPI_INVALID) {
		nrfx_gppi_event_endpoint_setup(ppi_channel, evt_address);
	}
	m_hw_task.ppi = ppi_channel;
	m_hw_task.fire_lpticks = fire_lpticks;
	nrf_802154_sl_mcu_critical_exit(mcu_cs_state);
	hw_task_state_set(HW_TASK_STATE_SETTING_UP, HW_TASK_STATE_READY);
	return NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_cleanup(void)
{
	if (!hw_task_state_set(HW_TASK_STATE_READY, HW_TASK_STATE_CLEANING)) {
		return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
	}

	hw_task_rtc_timer_abort();

	hw_task_rtc_cc_unbind(m_hw_task.chan, m_hw_task.ppi);
	m_hw_task.ppi = NRF_802154_SL_HW_TASK_PPI_INVALID;

	hw_task_state_set(HW_TASK_STATE_CLEANING, HW_TASK_STATE_IDLE);

	return NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_update_ppi(
	uint32_t ppi_channel)
{
	bool cc_triggered;

	if (!hw_task_state_set(HW_TASK_STATE_READY, HW_TASK_STATE_UPDATING)) {
		return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
	}

	nrf_802154_sl_mcu_critical_state_t mcu_cs_state;

	nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

	hw_task_rtc_cc_bind_to_ppi(m_hw_task.chan, ppi_channel);
	m_hw_task.ppi = ppi_channel;

	cc_triggered = hw_task_rtc_cc_event_check(m_hw_task.chan);
	if (z_nrf_rtc_timer_read() >= m_hw_task.fire_lpticks) {
		cc_triggered = true;
	}
	nrf_802154_sl_mcu_critical_exit(mcu_cs_state);

	hw_task_state_set(HW_TASK_STATE_UPDATING, HW_TASK_STATE_READY);

	return cc_triggered ? NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE :
	       NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}
