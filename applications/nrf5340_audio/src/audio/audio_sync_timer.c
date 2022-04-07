/*************************************************************************************************/
/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *  All rights reserved.
 */
/*************************************************************************************************/

/*
 * Redistribution and use of the Audio subsystem for nRF5340 Software, in binary
 * and source code forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * 1. Redistributions of source code form must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary code form, except as embedded into a Nordic
 *    Semiconductor ASA nRF53 chip or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of Packetcraft, Inc. nor Nordic Semiconductor ASA nor
 *    the names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA nRF53 chip.
 *
 * 5. Any software provided in binary or source code form under this license
 *    must not be reverse engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY PACKETCRAFT, INC. AND NORDIC SEMICONDUCTOR ASA
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL PACKETCRAFT, INC.,
 * NORDIC SEMICONDUCTOR ASA, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "audio_sync_timer.h"

#include <zephyr.h>
#include <nrfx_timer.h>
#include <nrfx_dppi.h>
#include <nrfx_i2s.h>
#include <nrfx_ipc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(audio_sync_timer, CONFIG_LOG_AUDIO_SYNC_TIMER_LEVEL);

#define AUDIO_SYNC_TIMER_INSTANCE 1

#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL 0
#define AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL 1

#define AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE NRF_TIMER_TASK_CAPTURE0

#define AUDIO_SYNC_TIMER_NET_APP_IPC_EVT NRF_IPC_EVENT_RECEIVE_4
#define AUDIO_SYNC_TIMER_NET_APP_IPC_SIGNAL_IDX 4

static const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(AUDIO_SYNC_TIMER_INSTANCE);

static uint8_t dppi_channel_timer_clear;
static uint8_t dppi_channel_i2s_frame_start;

static nrfx_timer_config_t cfg = { .frequency = NRF_TIMER_FREQ_1MHz,
				   .mode = NRF_TIMER_MODE_TIMER,
				   .bit_width = NRF_TIMER_BIT_WIDTH_32,
				   .interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
				   .p_context = NULL };

static void event_handler(nrf_timer_event_t event_type, void *ctx)
{
}

uint32_t audio_sync_timer_i2s_frame_start_ts_get(void)
{
	return nrfx_timer_capture_get(&timer_instance,
				      AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE_CHANNEL);
}

uint32_t audio_sync_timer_curr_time_get(void)
{
	return nrfx_timer_capture(&timer_instance, AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL);
}

void audio_sync_timer_sync_evt_send(void)
{
	nrfx_ipc_signal(AUDIO_SYNC_TIMER_NET_APP_IPC_SIGNAL_IDX);
}

int audio_sync_timer_init(void)
{
	nrfx_err_t ret;

	ret = nrfx_timer_init(&timer_instance, &cfg, event_handler);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx timer init error - Return value: %d", ret);
		return ret;
	}
	nrfx_timer_enable(&timer_instance);

	/* Initialize capturing of I2S frame start event timestamps */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_i2s_frame_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (I2S frame start) - Return value: %d", ret);
		return ret;
	}
	nrf_timer_subscribe_set(timer_instance.p_reg, AUDIO_SYNC_TIMER_I2S_FRAME_START_EVT_CAPTURE,
				dppi_channel_i2s_frame_start);
	nrf_i2s_publish_set(NRF_I2S0, NRF_I2S_EVENT_FRAMESTART, dppi_channel_i2s_frame_start);
	ret = nrfx_dppi_channel_enable(dppi_channel_i2s_frame_start);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (I2S frame start) - Return value: %d", ret);
		return ret;
	}

	/* Initialize functionality for synchronization between APP and NET core */
	ret = nrfx_dppi_channel_alloc(&dppi_channel_timer_clear);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel alloc error (timer clear) - Return value: %d", ret);
		return ret;
	}
	nrf_ipc_publish_set(NRF_IPC, AUDIO_SYNC_TIMER_NET_APP_IPC_EVT, dppi_channel_timer_clear);
	nrf_timer_subscribe_set(timer_instance.p_reg, NRF_TIMER_TASK_CLEAR,
				dppi_channel_timer_clear);
	ret = nrfx_dppi_channel_enable(dppi_channel_timer_clear);
	if (ret - NRFX_ERROR_BASE_NUM) {
		LOG_ERR("nrfx DPPI channel enable error (timer clear) - Return value: %d", ret);
		return ret;
	}

	return 0;
}
