/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/nrf_802154_platform_sl_lptimer.h"

#include <assert.h>
#include <zephyr/kernel.h>

#include "drivers/timer/nrf_rtc_timer.h"

#include "platform/nrf_802154_clock.h"
#include "nrf_802154_sl_utils.h"


struct timer_desc {
	z_nrf_rtc_timer_compare_handler_t handler;
	uint64_t target_time;
	int32_t chan;
	int32_t int_lock_key;
};

static struct timer_desc m_timer;
static struct timer_desc m_sync_timer;
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

void nrf_802154_platform_sl_lp_timer_init(void)
{
	m_in_critical_section = false;
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
