/*
 * Copyright (c) 2018 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file contains implementation of the nRF 802.15.4 high precision timer abstraction.
 *
 * This implementation is built on top of the TIMER peripheral.
 * If SoftDevice RAAL is in use the TIMER peripheral is shared between RAAL and this module.
 *
 */

#include "platform/nrf_802154_hp_timer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "hal/nrf_timer.h"

#include "nrf_802154_sl_periphs.h"

/**@brief Timer instance. */
#define TIMER                 NRF_802154_HIGH_PRECISION_TIMER_INSTANCE

/**@brief Timer compare channel definitions. */
#define TIMER_CC_CAPTURE      NRF_TIMER_CC_CHANNEL1
#define TIMER_CC_CAPTURE_TASK NRF_TIMER_TASK_CAPTURE1

#define TIMER_CC_SYNC         NRF_TIMER_CC_CHANNEL2
#define TIMER_CC_SYNC_TASK    NRF_TIMER_TASK_CAPTURE2
#define TIMER_CC_SYNC_EVENT   NRF_TIMER_EVENT_COMPARE2
#define TIMER_CC_SYNC_INT     NRF_TIMER_INT_COMPARE2_MASK

#define TIMER_CC_EVT          NRF_TIMER_CC_CHANNEL3
#define TIMER_CC_EVT_TASK     NRF_TIMER_TASK_CAPTURE3
#define TIMER_CC_EVT_INT      NRF_TIMER_INT_COMPARE3_MASK

/**@brief Unexpected value in the sync compare channel. */
static uint32_t m_unexpected_sync;

/**@brief Get current time on the Timer. */
static inline uint32_t timer_time_get(void)
{
	nrf_timer_task_trigger(TIMER, TIMER_CC_CAPTURE_TASK);
	return nrf_timer_cc_get(TIMER, TIMER_CC_CAPTURE);
}

void nrf_802154_hp_timer_init(void)
{
	nrf_timer_bit_width_set(TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_prescaler_set(TIMER, NRF_TIMER_FREQ_1MHz);
	nrf_timer_mode_set(TIMER, NRF_TIMER_MODE_TIMER);
}

void nrf_802154_hp_timer_deinit(void)
{
	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_SHUTDOWN);
}

void nrf_802154_hp_timer_start(void)
{
	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_START);
}

void nrf_802154_hp_timer_stop(void)
{
	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_SHUTDOWN);
}

uint32_t nrf_802154_hp_timer_sync_task_get(void)
{
	return nrf_timer_task_address_get(TIMER, TIMER_CC_SYNC_TASK);
}

void nrf_802154_hp_timer_sync_prepare(void)
{
	uint32_t past_time = timer_time_get() - 1;

	m_unexpected_sync = past_time;
	nrf_timer_cc_set(TIMER, TIMER_CC_SYNC, past_time);
}

bool nrf_802154_hp_timer_sync_time_get(uint32_t *p_timestamp)
{
	bool result = false;
	uint32_t sync_time = nrf_timer_cc_get(TIMER, TIMER_CC_SYNC);

	assert(p_timestamp != NULL);

	if (sync_time != m_unexpected_sync) {
		*p_timestamp = sync_time;
		result = true;
	}

	return result;
}

uint32_t nrf_802154_hp_timer_timestamp_task_get(void)
{
	return nrf_timer_task_address_get(TIMER, TIMER_CC_EVT_TASK);
}

uint32_t nrf_802154_hp_timer_timestamp_get(void)
{
	return nrf_timer_cc_get(TIMER, TIMER_CC_EVT);
}

uint32_t nrf_802154_hp_timer_current_time_get(void)
{
	return timer_time_get();
}
