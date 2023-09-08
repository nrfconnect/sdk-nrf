/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <nrfx_dppi.h>
#include <nrfx_i2s.h>
#include <nrfx_ipc.h>
#include <nrfx_rtc.h>
#include <nrfx_timer.h>
#include <nrfx_egu.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_sync_timer, CONFIG_AUDIO_SYNC_TIMER_LOG_LEVEL);

#include "audio_sync_timer.h"

#define AUDIO_RTC_TIMER_INSTANCE_NUMBER                     0
#define AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE         NRF_RTC_TASK_CAPTURE_0
#define AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_CHANNEL           1
#define AUDIO_RTC_TIMER_CURR_TIME_CAPTURE                   NRF_RTC_TASK_CAPTURE_1

const nrfx_rtc_t audio_rtc_timer_instance =
	NRFX_RTC_INSTANCE(AUDIO_RTC_TIMER_INSTANCE_NUMBER);

static nrfx_rtc_config_t rtc_cfg = NRFX_RTC_DEFAULT_CONFIG;

static void rtc_handler(nrfx_rtc_int_type_t int_type)
{
}

#define AUDIO_SYNC_TIMER_INSTANCE_NUMBER                     1
#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE         NRF_TIMER_TASK_CAPTURE0
#define AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL           1
#define AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE                   NRF_TIMER_TASK_CAPTURE1

static const nrfx_timer_t audio_sync_timer_instance =
	NRFX_TIMER_INSTANCE(AUDIO_SYNC_TIMER_INSTANCE_NUMBER);

static nrfx_timer_config_t cfg = {.frequency = NRFX_MHZ_TO_HZ(1UL),
				  .mode = NRF_TIMER_MODE_TIMER,
				  .bit_width = NRF_TIMER_BIT_WIDTH_32,
				  .interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
				  .p_context = NULL};

static void event_handler(nrf_timer_event_t event_type, void *ctx)
{
}

#define AUDIO_SYNC_TIMER_NET_APP_IPC_START_EVT NRF_IPC_EVENT_RECEIVE_4
#define AUDIO_SYNC_TIMER_NET_APP_IPC_STOP_EVT  NRF_IPC_EVENT_RECEIVE_5

static uint8_t dppi_channel_timer_start;
static uint8_t dppi_channel_timer_stop;
static uint8_t dppi_channel_timer_sync;
static uint8_t dppi_channel_i2s_frame_start;
#define USE_DPPI 1
#if USE_DPPI
static uint8_t dppi_channel_curr_time_capture;
#endif

uint32_t audio_sync_timer_capture(void)
{
	uint32_t remainder_us;
	uint32_t tick;

#if USE_DPPI
	NRF_EGU0->TASKS_TRIGGER[0] = 1U;
#else
	NRF_RTC0->TASKS_CAPTURE[AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_CHANNEL] = 1U;
	NRF_TIMER1->TASKS_CAPTURE[AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL] = 1U;
#endif

	tick = NRF_RTC0->CC[AUDIO_RTC_TIMER_CURR_TIME_CAPTURE_CHANNEL];
	remainder_us = NRF_TIMER1->CC[AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL];

	return (((uint64_t)tick * 30517578125UL) / 1000000000UL) + remainder_us;
}

uint32_t audio_sync_timer_capture_get(void)
{
	uint32_t remainder_us;
	uint32_t tick;

	tick = NRF_RTC0->CC[AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL];
	remainder_us = NRF_TIMER1->CC[AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL];

	return (((uint64_t)tick * 30517578125UL) / 1000000000UL) + remainder_us;
}

/**
 * @brief Initialize audio sync timer
 *
 * @note Clearing of the nRF5340 APP core sync
 * timer is initialized here. The sync timers on
 * APP core and NET core are cleared at exactly
 * the same time using an IPC signal sent from
 * the NET core. This makes the two timers
 * synchronized.
 *
 * @param unused Unused
 *
 * @return 0 if successful, error otherwise
 */
static int audio_sync_timer_init(void)
{
	nrfx_err_t ret;

	ret = nrfx_rtc_init(&audio_rtc_timer_instance, &rtc_cfg, rtc_handler);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx rtc init error - Return value: %d", ret);
		return ret;
	}

	ret = nrfx_timer_init(&audio_sync_timer_instance, &cfg, event_handler);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx timer init error - Return value: %d", ret);
		return ret;
	}

	nrfx_timer_enable(&audio_sync_timer_instance);

	/* Initialize capturing of I2S frame start event timestamps */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_i2s_frame_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start) - Return value: %d", ret);
		return ret;
	}

	nrf_rtc_subscribe_set(audio_rtc_timer_instance.p_reg,
			      AUDIO_RTC_TIMER_I2S_FRAME_START_EVT_CAPTURE,
			      dppi_channel_i2s_frame_start);

	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg,
				AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE,
				dppi_channel_i2s_frame_start);

	nrf_i2s_publish_set(NRF_I2S0, NRF_I2S_EVENT_FRAMESTART, dppi_channel_i2s_frame_start);

	ret = nrfx_dppi_channel_enable(dppi_channel_i2s_frame_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (I2S frame start) - Return value: %d", ret);
		return ret;
	}

#if USE_DPPI
	/* Initialize capturing of current timestamps */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_curr_time_capture);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start) - Return value: %d", ret);
		return ret;
	}

	nrf_rtc_subscribe_set(audio_rtc_timer_instance.p_reg,
			      AUDIO_RTC_TIMER_CURR_TIME_CAPTURE,
			      dppi_channel_curr_time_capture);

	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg,
				AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE,
				dppi_channel_curr_time_capture);

	nrf_egu_publish_set(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0, dppi_channel_curr_time_capture);

	ret = nrfx_dppi_channel_enable(dppi_channel_curr_time_capture);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (I2S frame start) - Return value: %d", ret);
		return ret;
	}
#endif

	/* Initialize functionality for synchronization between APP and NET core */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_timer_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (timer clear) - Return value: %d", ret);
		return ret;
	}

	nrf_ipc_publish_set(NRF_IPC, AUDIO_SYNC_TIMER_NET_APP_IPC_START_EVT,
			    dppi_channel_timer_start);

	nrf_rtc_subscribe_set(audio_rtc_timer_instance.p_reg, NRF_RTC_TASK_START,
			      dppi_channel_timer_start);

	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg, NRF_TIMER_TASK_CLEAR,
				dppi_channel_timer_start);

	ret = nrfx_dppi_channel_enable(dppi_channel_timer_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (timer clear) - Return value: %d", ret);
		return ret;
	}

	/* Initialize functionality for synchronization between APP and NET core */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_timer_stop);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (timer clear) - Return value: %d", ret);
		return ret;
	}

	nrf_ipc_publish_set(NRF_IPC, AUDIO_SYNC_TIMER_NET_APP_IPC_STOP_EVT,
			    dppi_channel_timer_stop);

	nrf_rtc_subscribe_set(audio_rtc_timer_instance.p_reg, NRF_RTC_TASK_STOP,
			      dppi_channel_timer_stop);

	ret = nrfx_dppi_channel_enable(dppi_channel_timer_stop);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (timer clear) - Return value: %d", ret);
		return ret;
	}

	/* Initialize functionality for synchronization between RTC and TIMER */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_timer_sync);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (timer clear) - Return value: %d", ret);
		return ret;
	}

	nrf_rtc_publish_set(NRF_RTC0, NRF_RTC_EVENT_TICK, dppi_channel_timer_sync);

	nrf_timer_subscribe_set(audio_sync_timer_instance.p_reg, NRF_TIMER_TASK_CLEAR,
				dppi_channel_timer_sync);

	ret = nrfx_dppi_channel_enable(dppi_channel_timer_sync);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (timer clear) - Return value: %d", ret);
		return ret;
	}

	nrfx_rtc_tick_enable(&audio_rtc_timer_instance, false);

	LOG_DBG("Audio sync timer initialized");

	return 0;
}

SYS_INIT(audio_sync_timer_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
