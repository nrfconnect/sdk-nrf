/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_sync_timer.h"

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_i2s.h>
#include <nrfx_ipc.h>
#include <nrfx_rtc.h>
#include <nrfx_timer.h>
#include <nrfx_egu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_sync_timer, CONFIG_AUDIO_SYNC_TIMER_LOG_LEVEL);

#define AUDIO_SYNC_TIMER_NET_APP_IPC_EVT_CHANNEL 4
#define AUDIO_SYNC_TIMER_NET_APP_IPC_EVT	 NRF_IPC_EVENT_RECEIVE_4

#define AUDIO_SYNC_HF_TIMER_INSTANCE_NUMBER 1

#define AUDIO_SYNC_HF_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_HF_TIMER_I2S_FRAME_START_EVT_CAPTURE		NRF_TIMER_TASK_CAPTURE0
#define AUDIO_SYNC_HF_TIMER_CURR_TIME_CAPTURE_CHANNEL		1
#define AUDIO_SYNC_HF_TIMER_CURR_TIME_CAPTURE			NRF_TIMER_TASK_CAPTURE1

static nrfx_timer_t audio_sync_hf_timer_instance =
	NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(AUDIO_SYNC_HF_TIMER_INSTANCE_NUMBER));

static nrfx_gppi_handle_t dppi_handle_i2s_frame_start;

#define AUDIO_SYNC_LF_TIMER_INSTANCE NRF_RTC0

#define AUDIO_SYNC_LF_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_LF_TIMER_I2S_FRAME_START_EVT_CAPTURE		NRF_RTC_TASK_CAPTURE_0
#define AUDIO_SYNC_LF_TIMER_CURR_TIME_CAPTURE_CHANNEL		1
#define AUDIO_SYNC_LF_TIMER_CURR_TIME_CAPTURE			NRF_RTC_TASK_CAPTURE_1
#define CC_GET_CALLS_MAX					20

static nrfx_gppi_handle_t dppi_handle_curr_time_capture;

static const nrfx_rtc_config_t rtc_cfg = NRFX_RTC_DEFAULT_CONFIG;

static nrfx_rtc_t audio_sync_lf_timer_instance = NRFX_RTC_INSTANCE(AUDIO_SYNC_LF_TIMER_INSTANCE);

static nrfx_gppi_handle_t dppi_handle_timer_sync_with_rtc;
static nrfx_gppi_handle_t dppi_handle_rtc_start;
static volatile uint32_t num_rtc_overflows;

static nrfx_timer_config_t cfg = {.frequency = NRFX_MHZ_TO_HZ(1UL),
				  .mode = NRF_TIMER_MODE_TIMER,
				  .bit_width = NRF_TIMER_BIT_WIDTH_32,
				  .interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
				  .p_context = NULL};

static uint32_t timestamp_from_rtc_and_timer_get(uint32_t ticks, uint32_t remainder_us)
{
	const uint64_t rtc_ticks_in_femto_units = 30517578125UL;
	const uint32_t rtc_overflow_time_us = 512000000UL;

	return ((ticks * rtc_ticks_in_femto_units) / 1000000000UL) +
	       (num_rtc_overflows * rtc_overflow_time_us) + remainder_us;
}

uint32_t audio_sync_timer_capture(void)
{
	/* Ensure that the follow product specification statement is handled:
	 *
	 * There is a delay of 6 PCLK16M periods from when the TASKS_CAPTURE[n] is triggered
	 * until the corresponding CC[n] register is updated.
	 *
	 * Lets have a stale value in the CC[n] register and compare that it is different when
	 * we capture using DPPI.
	 *
	 * We ensure it is stale by setting it as the previous tick relative to current
	 * counter value.
	 */
	uint32_t tick_stale = nrf_rtc_counter_get(audio_sync_lf_timer_instance.p_reg);

	/* Set a stale value in the CC[n] register */
	tick_stale--;
	nrf_rtc_cc_set(audio_sync_lf_timer_instance.p_reg,
		       AUDIO_SYNC_LF_TIMER_CURR_TIME_CAPTURE_CHANNEL, tick_stale);

	/* Trigger EGU task to capture RTC and TIMER value */
	nrf_egu_task_trigger(NRF_EGU0, NRF_EGU_TASK_TRIGGER0);

	/* Read captured RTC value */
	uint32_t tick = nrf_rtc_cc_get(audio_sync_lf_timer_instance.p_reg,
				       AUDIO_SYNC_LF_TIMER_CURR_TIME_CAPTURE_CHANNEL);

	/* If required, wait until CC[n] register is updated */
	while (tick == tick_stale) {
		tick = nrf_rtc_cc_get(audio_sync_lf_timer_instance.p_reg,
				      AUDIO_SYNC_LF_TIMER_CURR_TIME_CAPTURE_CHANNEL);
	}

	/* Read captured TIMER value */
	uint32_t remainder_us =
		nrf_timer_cc_get(NRF_TIMER1, AUDIO_SYNC_HF_TIMER_CURR_TIME_CAPTURE_CHANNEL);

	return timestamp_from_rtc_and_timer_get(tick, remainder_us);
}

uint32_t audio_sync_timer_frame_start_capture_get(void)
{
	uint32_t cc_get_calls = 0;
	uint32_t tick = 0;
	static uint32_t prev_tick;
	uint32_t remainder_us = 0;

	/* This ISR may be called before the I2S FRAMESTART event which does timer capture,
	 * resulting in values not yet being updated in the *_cc_get calls.
	 * To ensure new values are fetched, they are read in a while-loop with a timeout.
	 * Depending on variables such as sample frequency, this delay varies.
	 * For 16 kHz sampling rate is about 20 us. Less for 24 kHz and 48 kHz.
	 * Ref: OCT-2585 and OCT-3368.
	 */

	tick = nrf_rtc_cc_get(audio_sync_lf_timer_instance.p_reg,
			      AUDIO_SYNC_LF_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL);

	while (tick == prev_tick) {
		k_busy_wait(1);
		tick = nrf_rtc_cc_get(audio_sync_lf_timer_instance.p_reg,
				      AUDIO_SYNC_LF_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL);
		cc_get_calls++;
		if (cc_get_calls > CC_GET_CALLS_MAX) {
			LOG_ERR("Unable to get new tick from nrf_rtc_cc_get");
			break;
		}
	};

	cc_get_calls = 0;

	/* The HF timer is cleared on every I2S frame. Hence, this value can be
	 * the same many times in a row.
	 */
	remainder_us = nrf_timer_cc_get(NRF_TIMER1,
					AUDIO_SYNC_HF_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL);

	prev_tick = tick;

	return timestamp_from_rtc_and_timer_get(tick, remainder_us);
}

static void unused_timer_isr_handler(nrf_timer_event_t event_type, void *ctx)
{
	ARG_UNUSED(event_type);
	ARG_UNUSED(ctx);
}

static void rtc_isr_handler(nrf_rtc_event_t event_type, void *ctx)
{
	ARG_UNUSED(ctx);

	if (event_type == NRF_RTC_EVENT_OVERFLOW) {
		num_rtc_overflows++;
	}
}

/**
 * @brief Initialize audio sync timer
 *
 * @note The audio sync timers is replicating the controller's clock.
 * The controller starts or clears the sync timer using a PPI signal
 * sent from the controller. This makes the two clocks synchronized.
 *
 * @return 0 if successful, error otherwise
 */
static int audio_sync_timer_init(void)
{
	int ret;
	uint32_t eep0, tep0, tep1;

	ret = nrfx_timer_init(&audio_sync_hf_timer_instance, &cfg, unused_timer_isr_handler);
	if (ret < 0) {
		LOG_ERR("nrfx timer init error: %d", ret);
		return -ENODEV;
	}

	ret = nrfx_rtc_init(&audio_sync_lf_timer_instance, &rtc_cfg, rtc_isr_handler);
	if (ret < 0) {
		LOG_ERR("nrfx rtc init error: %d", ret);
		return -ENODEV;
	}

	IRQ_CONNECT(RTC0_IRQn, IRQ_PRIO_LOWEST, nrfx_rtc_irq_handler,
			&audio_sync_lf_timer_instance, 0);
	nrfx_rtc_overflow_enable(&audio_sync_lf_timer_instance, true);

	/* Initialize capturing of I2S frame start event timestamps */
	eep0 = nrf_i2s_event_address_get(NRF_I2S0, NRF_I2S_EVENT_FRAMESTART);
	tep0 = nrfx_rtc_task_address_get(&audio_sync_lf_timer_instance,
			AUDIO_SYNC_LF_TIMER_I2S_FRAME_START_EVT_CAPTURE);
	tep1 = nrfx_timer_task_address_get(&audio_sync_hf_timer_instance,
			AUDIO_SYNC_HF_TIMER_I2S_FRAME_START_EVT_CAPTURE);

	ret = nrfx_gppi_conn_alloc(eep0, tep0, &dppi_handle_i2s_frame_start);
	if (ret < 0) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start): %d", ret);
		return ret;
	}

	nrfx_gppi_ep_attach(tep1, dppi_handle_i2s_frame_start);
	nrfx_gppi_conn_enable(dppi_handle_i2s_frame_start);

	/* Initialize capturing of current timestamps */
	eep0 = nrf_egu_event_address_get(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0);
	tep0 = nrfx_rtc_task_address_get(&audio_sync_lf_timer_instance,
			AUDIO_SYNC_LF_TIMER_CURR_TIME_CAPTURE);
	tep1 = nrfx_timer_task_address_get(&audio_sync_hf_timer_instance,
			AUDIO_SYNC_HF_TIMER_CURR_TIME_CAPTURE);

	ret = nrfx_gppi_conn_alloc(eep0, tep0, &dppi_handle_curr_time_capture);
	if (ret < 0) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start): %d", ret);
		return ret;
	}
	nrfx_gppi_ep_attach(tep1, dppi_handle_curr_time_capture);
	nrfx_gppi_conn_enable(dppi_handle_curr_time_capture);

	/* Initialize functionality for synchronization between APP and NET core */
	eep0 = nrf_ipc_event_address_get(NRF_IPC, AUDIO_SYNC_TIMER_NET_APP_IPC_EVT);
	tep0 = nrfx_rtc_task_address_get(&audio_sync_lf_timer_instance, NRF_RTC_TASK_CLEAR);
	tep1 = nrfx_timer_task_address_get(&audio_sync_hf_timer_instance, NRF_TIMER_TASK_START);
	ret = nrfx_gppi_conn_alloc(eep0, tep0, &dppi_handle_rtc_start);
	if (ret < 0) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start): %d", ret);
		return ret;
	}
	nrfx_gppi_ep_attach(tep1, dppi_handle_rtc_start);
	nrf_ipc_receive_config_set(NRF_IPC, AUDIO_SYNC_TIMER_NET_APP_IPC_EVT_CHANNEL,
				   NRF_IPC_CHANNEL_4);
	nrfx_gppi_conn_enable(dppi_handle_rtc_start);

	/* Initialize functionality for synchronization between RTC and TIMER */
	eep0 = nrfx_rtc_event_address_get(&audio_sync_lf_timer_instance, NRF_RTC_EVENT_TICK);
	tep0 = nrfx_timer_task_address_get(&audio_sync_hf_timer_instance, NRF_TIMER_TASK_CLEAR);
	ret = nrfx_gppi_conn_alloc(eep0, tep0, &dppi_handle_timer_sync_with_rtc);
	if (ret < 0) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start): %d", ret);
		return ret;
	}

	nrfx_rtc_tick_enable(&audio_sync_lf_timer_instance, false);
	nrfx_gppi_conn_enable(dppi_handle_timer_sync_with_rtc);

	nrfx_rtc_enable(&audio_sync_lf_timer_instance);

	LOG_DBG("Audio sync timer initialized");

	return 0;
}

SYS_INIT(audio_sync_timer_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
