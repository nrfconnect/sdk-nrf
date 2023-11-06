/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_sync_timer.h"

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <nrfx_dppi.h>
#include <nrfx_i2s.h>
#include <nrfx_ipc.h>
#include <nrfx_rtc.h>
#include <nrfx_timer.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_sync_timer, CONFIG_AUDIO_SYNC_TIMER_LOG_LEVEL);

#define AUDIO_SYNC_TIMER_INSTANCE_NUMBER 1

#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL	     1
#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE NRF_TIMER_TASK_CAPTURE0

#define AUDIO_SYNC_TIMER_NET_APP_IPC_EVT_CHANNEL 4
#define AUDIO_SYNC_TIMER_NET_APP_IPC_EVT NRF_IPC_EVENT_RECEIVE_4

static const nrfx_timer_t audio_sync_timer_instance =
	NRFX_TIMER_INSTANCE(AUDIO_SYNC_TIMER_INSTANCE_NUMBER);

static uint8_t dppi_channel_i2s_frame_start;

#define AUDIO_RTC_TIMER_INSTANCE_NUMBER                     0

#define AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_CHANNEL           1
#define AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE         NRF_RTC_TASK_CAPTURE_0
#define AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_TASK              NRF_RTC_TASK_CAPTURE_1

static const nrfx_rtc_config_t rtc_cfg = NRFX_RTC_DEFAULT_CONFIG;

static const nrfx_rtc_t audio_rtc_timer_instance =
	NRFX_RTC_INSTANCE(AUDIO_RTC_TIMER_INSTANCE_NUMBER);

static uint8_t dppi_channel_timer_sync_with_rtc;
static uint8_t dppi_channel_rtc_start;
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
		(num_rtc_overflows * rtc_overflow_time_us) +
		remainder_us;
}

uint32_t audio_sync_timer_capture(void)
{
	nrf_rtc_task_trigger(audio_rtc_timer_instance.p_reg,
				AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_TASK);
	nrf_timer_task_trigger(NRF_TIMER1,
		nrf_timer_capture_task_get(AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL));

	uint32_t tick = nrf_rtc_cc_get(audio_rtc_timer_instance.p_reg,
					AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_CHANNEL);
	uint32_t remainder_us = nrf_timer_cc_get(NRF_TIMER1,
						AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL);

	return timestamp_from_rtc_and_timer_get(tick, remainder_us);
}

uint32_t audio_sync_timer_capture_get(void)
{
	uint32_t remainder_us;
	uint32_t tick;

	tick = nrf_rtc_cc_get(audio_rtc_timer_instance.p_reg,
			AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL);
	remainder_us = nrf_timer_cc_get(NRF_TIMER1,
				AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL);

	return timestamp_from_rtc_and_timer_get(tick, remainder_us);
}

static void unused_timer_isr_handler(nrf_timer_event_t event_type, void *ctx)
{
	ARG_UNUSED(event_type);
	ARG_UNUSED(ctx);
}

static void rtc_isr_handler(nrfx_rtc_int_type_t int_type)
{
	if (int_type == NRFX_RTC_INT_OVERFLOW) {
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
	nrfx_err_t ret;

	ret = nrfx_timer_init(&audio_sync_timer_instance, &cfg, unused_timer_isr_handler);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx timer init error: %d", ret);
		return ret;
	}

	ret = nrfx_rtc_init(&audio_rtc_timer_instance, &rtc_cfg, rtc_isr_handler);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx rtc init error: %d", ret);
		return ret;
	}

	IRQ_CONNECT(RTC0_IRQn,
		IRQ_PRIO_LOWEST,
		nrfx_rtc_0_irq_handler,
		NULL,
		0);
	nrfx_rtc_overflow_enable(&audio_rtc_timer_instance, true);

	/* Initialize capturing of I2S frame start event timestamps */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_i2s_frame_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start): %d", ret);
		return ret;
	}

	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg,
				AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE,
				dppi_channel_i2s_frame_start);

	/* Initialize capturing of I2S frame start event timestamps at the RTC as well. */
	nrf_rtc_subscribe_set(audio_rtc_timer_instance.p_reg,
			AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE,
			dppi_channel_i2s_frame_start);

	nrf_i2s_publish_set(NRF_I2S0, NRF_I2S_EVENT_FRAMESTART, dppi_channel_i2s_frame_start);
	ret = nrfx_dppi_channel_enable(dppi_channel_i2s_frame_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (I2S frame start): %d", ret);
		return ret;
	}

	/* Initialize functionality for synchronization between APP and NET core */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_rtc_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (timer clear): %d", ret);
		return ret;
	}

	nrf_rtc_subscribe_set(audio_rtc_timer_instance.p_reg,
			NRF_RTC_TASK_START,
			dppi_channel_rtc_start);
	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg,
				NRF_TIMER_TASK_START,
				dppi_channel_rtc_start);

	nrf_ipc_receive_config_set(NRF_IPC,
				AUDIO_SYNC_TIMER_NET_APP_IPC_EVT_CHANNEL,
				NRF_IPC_CHANNEL_4);
	nrf_ipc_publish_set(NRF_IPC,
			AUDIO_SYNC_TIMER_NET_APP_IPC_EVT,
			dppi_channel_rtc_start);

	ret = nrfx_dppi_channel_enable(dppi_channel_rtc_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (timer clear): %d", ret);
		return ret;
	}

	/* Initialize functionality for synchronization between RTC and TIMER */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_timer_sync_with_rtc);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (timer clear): %d", ret);
		return ret;
	}

	nrf_rtc_publish_set(audio_rtc_timer_instance.p_reg,
			NRF_RTC_EVENT_TICK,
			dppi_channel_timer_sync_with_rtc);
	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg,
				NRF_TIMER_TASK_CLEAR,
				dppi_channel_timer_sync_with_rtc);

	nrfx_rtc_tick_enable(&audio_rtc_timer_instance, false);

	ret = nrfx_dppi_channel_enable(dppi_channel_timer_sync_with_rtc);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (timer clear): %d", ret);
		return ret;
	}

	LOG_DBG("Audio sync timer initialized");

	return 0;
}

SYS_INIT(audio_sync_timer_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
