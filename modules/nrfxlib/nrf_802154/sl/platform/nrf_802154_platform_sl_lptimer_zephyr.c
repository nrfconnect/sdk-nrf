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
#include <helpers/nrfx_gppi.h>

#include "platform/nrf_802154_clock.h"
#include "nrf_802154_sl_utils.h"

#define RTC_CHAN_INVALID (-1)

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

static bool hw_task_state_set(enum hw_task_state_type expected_state,
			      enum hw_task_state_type new_state)
{
	return atomic_cas(&m_hw_task.state, expected_state, new_state);
}

static void cc_bind_to_ppi(int32_t cc_num, uint32_t ppi_num)
{
	uint32_t event_address = z_nrf_rtc_timer_compare_evt_address_get(cc_num);

	nrfx_gppi_event_endpoint_setup(ppi_num, event_address);
}

static void cc_unbind(int32_t cc_num, uint32_t ppi_num)
{
	if (ppi_num != NRF_802154_SL_HW_TASK_PPI_INVALID) {
		uint32_t event_address = z_nrf_rtc_timer_compare_evt_address_get(cc_num);

		nrfx_gppi_event_endpoint_clear(ppi_num, event_address);
	}
}

static bool cc_event_check(int32_t cc_num)
{
	uint32_t event_address = z_nrf_rtc_timer_compare_evt_address_get(cc_num);

	return *(volatile uint32_t *)event_address;
}

void nrf_802154_platform_sl_lp_timer_init(void)
{
	m_in_critical_section = false;
	m_hw_task.state = HW_TASK_STATE_IDLE;
	m_hw_task.chan = RTC_CHAN_INVALID;
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
		z_nrf_rtc_timer_chan_free(m_hw_task.chan);
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

	/* Allocate the channel if not already done. */
	if (m_hw_task.chan == RTC_CHAN_INVALID) {
		/* The channel allocation cannot take place during module
		 * initialization, because for nRF53 it would run
		 * out of resources - some other module temporarily needs
		 * an rtc channel to initialize. For this reason, this is where
		 * the allocation is made.
		 * Once a channel has been successfully allocated, it will be held
		 * until nrf_802154_platform_sl_lp_timer_deinit is called.
		 */
		m_hw_task.chan = z_nrf_rtc_timer_chan_alloc();
		if (m_hw_task.chan < 0) {
			m_hw_task.chan = RTC_CHAN_INVALID;
			hw_task_state_set(HW_TASK_STATE_SETTING_UP, HW_TASK_STATE_IDLE);
			return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
		}
	}

	if (z_nrf_rtc_timer_set(m_hw_task.chan, fire_lpticks, NULL, NULL) != 0) {
		hw_task_state_set(HW_TASK_STATE_SETTING_UP, HW_TASK_STATE_IDLE);
		return NRF_802154_SL_LPTIMER_PLATFORM_TOO_DISTANT;
	}

	evt_address = z_nrf_rtc_timer_compare_evt_address_get(m_hw_task.chan);

	nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

	/* For triggering to take place a safe margin is 2 lpticks from `now`. */
	if ((z_nrf_rtc_timer_read() + 2) > fire_lpticks) {
		/* it is too late */
		nrf_802154_sl_mcu_critical_exit(mcu_cs_state);
		cc_unbind(m_hw_task.chan, ppi_channel);
		m_hw_task.ppi = NRF_802154_SL_HW_TASK_PPI_INVALID;
		z_nrf_rtc_timer_abort(m_hw_task.chan);
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

	z_nrf_rtc_timer_abort(m_hw_task.chan);

	cc_unbind(m_hw_task.chan, m_hw_task.ppi);
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

	cc_bind_to_ppi(m_hw_task.chan, ppi_channel);
	m_hw_task.ppi = ppi_channel;

	cc_triggered = cc_event_check(m_hw_task.chan);
	if (z_nrf_rtc_timer_read() >= m_hw_task.fire_lpticks) {
		cc_triggered = true;
	}
	nrf_802154_sl_mcu_critical_exit(mcu_cs_state);

	hw_task_state_set(HW_TASK_STATE_UPDATING, HW_TASK_STATE_READY);

	return cc_triggered ? NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE :
	       NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}
