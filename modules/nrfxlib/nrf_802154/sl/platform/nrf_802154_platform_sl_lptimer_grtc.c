/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "platform/nrf_802154_platform_sl_lptimer.h"
#include "nrf_802154_platform_sl_lptimer_grtc_hw_task.h"

#include <assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

#include <haly/nrfy_dppi.h>
#include <haly/nrfy_grtc.h>

#include "nrf_802154_sl_config.h"
#include "nrf_802154_sl_atomics.h"
#include "nrf_802154_sl_utils.h"
#include "timer/nrf_802154_timer_coord.h"
#include "nrf_802154_sl_periphs.h"

static atomic_t m_enabled;
static bool     m_compare_int_lock_key;
static uint32_t m_critical_section_cnt;
static int32_t  m_callbacks_cc_channel;
static int32_t  m_hw_task_cc_channel;

static inline bool is_lptimer_enabled(void)
{
	return atomic_test_bit(&m_enabled, 0);
}

static void timer_compare_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	(void)user_data;
	(void)expire_time;

	assert(id == m_callbacks_cc_channel);

	if (!is_lptimer_enabled()) {
		/* The interrupt is late. Ignore it */
		return;
	}

	uint64_t curr_ticks = z_nrf_grtc_timer_read();

	nrf_802154_sl_timer_handler(curr_ticks);
}

void nrf_802154_platform_sl_lp_timer_init(void)
{
	m_critical_section_cnt = 0UL;

	m_callbacks_cc_channel = z_nrf_grtc_timer_chan_alloc();
	assert(m_callbacks_cc_channel >= 0);

	m_hw_task_cc_channel = z_nrf_grtc_timer_chan_alloc();
	assert(m_hw_task_cc_channel >= 0);

	nrf_802154_platform_sl_lptimer_hw_task_cross_domain_connections_setup(m_hw_task_cc_channel);
}

void nrf_802154_platform_sl_lp_timer_deinit(void)
{
	(void)z_nrf_grtc_timer_compare_int_lock(m_callbacks_cc_channel);

	z_nrf_grtc_timer_chan_free(m_callbacks_cc_channel);
	z_nrf_grtc_timer_chan_free(m_hw_task_cc_channel);
}

uint64_t nrf_802154_platform_sl_lptimer_current_lpticks_get(void)
{
	return z_nrf_grtc_timer_read();
}

uint64_t nrf_802154_platform_sl_lptimer_us_to_lpticks_convert(uint64_t us, bool round_up)
{
	(void) round_up;

	return us;
}

uint64_t nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
	return lpticks;
}

void nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
	/* This function is not required to be reentrant, hence no critical section. */
	atomic_set_bit(&m_enabled, 0);

	z_nrf_grtc_timer_set(m_callbacks_cc_channel,
			     fire_lpticks,
			     timer_compare_handler,
			     NULL);
}

void nrf_802154_platform_sl_lptimer_disable(void)
{
	atomic_clear_bit(&m_enabled, 0);

	z_nrf_grtc_timer_abort(m_callbacks_cc_channel);
}

void nrf_802154_platform_sl_lptimer_critical_section_enter(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	m_critical_section_cnt++;

	if (m_critical_section_cnt == 1UL) {
		m_compare_int_lock_key = z_nrf_grtc_timer_compare_int_lock(m_callbacks_cc_channel);
	}

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_platform_sl_lptimer_critical_section_exit(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	assert(m_critical_section_cnt > 0UL);

	if (m_critical_section_cnt == 1UL) {
		z_nrf_grtc_timer_compare_int_unlock(m_callbacks_cc_channel, m_compare_int_lock_key);
	}

	m_critical_section_cnt--;

	nrf_802154_sl_mcu_critical_exit(state);
}

typedef uint8_t hw_task_state_t;
#define HW_TASK_STATE_IDLE       0u
#define HW_TASK_STATE_SETTING_UP 1u
#define HW_TASK_STATE_READY      2u
#define HW_TASK_STATE_CLEANING   3u
#define HW_TASK_STATE_UPDATING   4u

static volatile hw_task_state_t m_hw_task_state = HW_TASK_STATE_IDLE;
static uint64_t                 m_hw_task_fire_lpticks;

static bool hw_task_state_set(hw_task_state_t expected_state, hw_task_state_t new_state)
{
	return nrf_802154_sl_atomic_cas_u8(
		(uint8_t *)&m_hw_task_state, &expected_state, new_state);
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_prepare(
	uint64_t fire_lpticks,
	uint32_t ppi_channel)
{
	const uint64_t                     grtc_cc_minimum_margin = 1uLL;
	uint64_t                           syscnt_now;
	bool                               done_on_time = true;
	nrf_802154_sl_mcu_critical_state_t mcu_cs_state;

	if (!hw_task_state_set(HW_TASK_STATE_IDLE, HW_TASK_STATE_SETTING_UP)) {
		/* the only one available set of peripherals is already used */
		return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
	}

	nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_setup(
		ppi_channel, m_hw_task_cc_channel);

	m_hw_task_fire_lpticks = fire_lpticks;

	z_nrf_grtc_timer_set(m_hw_task_cc_channel, fire_lpticks, NULL, NULL);

	nrf_802154_sl_mcu_critical_enter(mcu_cs_state);

	/* @todo: can this read be done outside of critical section? */
	syscnt_now = z_nrf_grtc_timer_read();

	if (syscnt_now + grtc_cc_minimum_margin >= fire_lpticks) {
		/* it is too late */
		nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_clear();
		z_nrf_grtc_timer_abort(m_hw_task_cc_channel);
		done_on_time = false;
	}

	nrf_802154_sl_mcu_critical_exit(mcu_cs_state);

	hw_task_state_set(HW_TASK_STATE_SETTING_UP,
			  done_on_time ? HW_TASK_STATE_READY : HW_TASK_STATE_IDLE);

	return done_on_time ? NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS :
			      NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE;
}

nrf_802154_sl_lptimer_platform_result_t nrf_802154_platform_sl_lptimer_hw_task_cleanup(void)
{
	if (!hw_task_state_set(HW_TASK_STATE_READY, HW_TASK_STATE_CLEANING)) {
		return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
	}

	nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_clear();

	z_nrf_grtc_timer_abort(m_hw_task_cc_channel);

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

	nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_setup(
		ppi_channel, m_hw_task_cc_channel);

	cc_triggered = z_nrf_grtc_timer_compare_evt_check(m_hw_task_cc_channel);
	if (z_nrf_grtc_timer_read() >= m_hw_task_fire_lpticks) {
		cc_triggered = true;
	}

	hw_task_state_set(HW_TASK_STATE_UPDATING, HW_TASK_STATE_READY);

	return cc_triggered ? NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE :
		NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}
