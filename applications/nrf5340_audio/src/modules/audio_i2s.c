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

#include "audio_i2s.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <nrfx_i2s.h>
#include <nrfx_clock.h>

#include "audio_sync_timer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio_i2s, CONFIG_LOG_I2S_LEVEL);

#define I2S_NL DT_NODELABEL(i2s0)

#define HFCLKAUDIO_12_288_MHZ 0x9BAE

enum audio_i2s_state {
	AUDIO_I2S_STATE_UNINIT,
	AUDIO_I2S_STATE_IDLE,
	AUDIO_I2S_STATE_STARTED,
};

static enum audio_i2s_state state = AUDIO_I2S_STATE_UNINIT;

PINCTRL_DT_DEFINE(I2S_NL);

static nrfx_i2s_config_t cfg = {
	/* Pins are configured by pinctrl. */
	.skip_gpio_cfg = true,
	.skip_psel_cfg = true,
	.irq_priority = DT_IRQ(I2S_NL, priority),
	.mode = NRF_I2S_MODE_MASTER,
	.format = NRF_I2S_FORMAT_I2S,
	.alignment = NRF_I2S_ALIGN_LEFT,
#if (CONFIG_AUDIO_BIT_DEPTH_16)
	.sample_width = NRF_I2S_SWIDTH_16BIT,
	.mck_setup = 0x66666000,
	.ratio = NRF_I2S_RATIO_128X,
#elif (CONFIG_AUDIO_BIT_DEPTH_24)
	.sample_width = NRF_I2S_SWIDTH_24BIT,
	/* Clock mismatch warning: See CONFIG_AUDIO_24_BIT in KConfig */
	.mck_setup = 0x2BE2B000,
	.ratio = NRF_I2S_RATIO_48X,
#elif (CONFIG_AUDIO_BIT_DEPTH_32)
	.sample_width = NRF_I2S_SWIDTH_32BIT,
	.mck_setup = 0x66666000,
	.ratio = NRF_I2S_RATIO_128X,
#else
#error Invalid bit depth selected
#endif /* (CONFIG_AUDIO_BIT_DEPTH_16) */
	.channels = NRF_I2S_CHANNELS_STEREO,
	.clksrc = NRF_I2S_CLKSRC_ACLK,
	.enable_bypass = false,
};

static i2s_blk_comp_callback_t i2s_blk_comp_callback;

static void i2s_comp_handler(nrfx_i2s_buffers_t const *released_bufs, uint32_t status)
{
	if ((status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) && released_bufs &&
	    i2s_blk_comp_callback && (released_bufs->p_rx_buffer || released_bufs->p_tx_buffer)) {
		i2s_blk_comp_callback(audio_sync_timer_i2s_frame_start_ts_get(),
				      released_bufs->p_rx_buffer, released_bufs->p_tx_buffer);
	}
}

void audio_i2s_set_next_buf(const uint8_t *tx_buf, uint32_t *rx_buf)
{
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_STARTED);
	__ASSERT_NO_MSG(rx_buf != NULL);
#if (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET))
	__ASSERT_NO_MSG(tx_buf != NULL);
#endif /* (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET)) */

	const nrfx_i2s_buffers_t i2s_buf = { .p_rx_buffer = rx_buf,
					     .p_tx_buffer = (uint32_t *)tx_buf };

	nrfx_err_t ret;

	ret = nrfx_i2s_next_buffers_set(&i2s_buf);
	__ASSERT_NO_MSG(ret == NRFX_SUCCESS);
}

void audio_i2s_start(const uint8_t *tx_buf, uint32_t *rx_buf)
{
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_IDLE);
	__ASSERT_NO_MSG(rx_buf != NULL);
#if (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET))
	__ASSERT_NO_MSG(tx_buf != NULL);
#endif /* (CONFIG_STREAM_BIDIRECTIONAL || (CONFIG_AUDIO_DEV == HEADSET)) */

	const nrfx_i2s_buffers_t i2s_buf = { .p_rx_buffer = rx_buf,
					     .p_tx_buffer = (uint32_t *)tx_buf };

	nrfx_err_t ret;

	/* Buffer size in 32-bit words */
	ret = nrfx_i2s_start(&i2s_buf, I2S_SAMPLES_NUM, 0);
	__ASSERT_NO_MSG(ret == NRFX_SUCCESS);

	state = AUDIO_I2S_STATE_STARTED;
}

void audio_i2s_stop(void)
{
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_STARTED);

	nrfx_i2s_stop();

	state = AUDIO_I2S_STATE_IDLE;
}

void audio_i2s_blk_comp_cb_register(i2s_blk_comp_callback_t blk_comp_callback)
{
	i2s_blk_comp_callback = blk_comp_callback;
}

void audio_i2s_init(void)
{
	__ASSERT_NO_MSG(state == AUDIO_I2S_STATE_UNINIT);

	nrfx_err_t ret;

	nrfx_clock_hfclkaudio_config_set(HFCLKAUDIO_12_288_MHZ);

	NRF_CLOCK->TASKS_HFCLKAUDIOSTART = 1;

	/* Wait for ACLK to start */
	while (!NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED) {
		k_sleep(K_MSEC(1));
	}

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(I2S_NL), PINCTRL_STATE_DEFAULT);
	__ASSERT_NO_MSG(ret == 0);

	IRQ_CONNECT(DT_IRQN(I2S_NL), DT_IRQ(I2S_NL, priority), nrfx_isr, nrfx_i2s_irq_handler, 0);
	irq_enable(DT_IRQN(I2S_NL));

	ret = nrfx_i2s_init(&cfg, i2s_comp_handler);
	__ASSERT_NO_MSG(ret == NRFX_SUCCESS);

	state = AUDIO_I2S_STATE_IDLE;
}
