/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sw_codec_g722.h"

#include <errno.h>
#include <string.h>
#include <sample_rate_converter.h>
#include <zephyr/logging/log.h>

#include "sw_codec_select.h"
#include "g722_encoder.h"
#include "g722_decoder.h"

LOG_MODULE_REGISTER(sw_codec_g722, CONFIG_SW_CODEC_SELECT_LOG_LEVEL);

/* G722_DEFAULT: 16 kHz linear PCM in/out, one 8-bit code word per two samples at 64 kbps */
#define G722_PCM_SAMPLES_PER_OCTET 2
#define G722_PCM_FRAME_BYTES                                                                       \
	(G722_ENC_MONO_FRAME_SIZE * G722_PCM_SAMPLES_PER_OCTET * sizeof(int16_t))

/* The library defines both context types as void. */
static void *g722_enc_ctx[CONFIG_AUDIO_ENCODE_CHANNELS_MAX];
static void *g722_dec_ctx[CONFIG_AUDIO_DECODE_CHANNELS_MAX];

void sw_codec_g722_enc_uninit_all(void);
void sw_codec_g722_dec_uninit_all(void);

/**
 * @brief Allocate one independent encoder context per channel.
 *
 * @pre No encoder direction is currently initialized.
 * @pre Profile and channel count have been validated by sw_codec_g722_init().
 *
 * @param[in]	num_ch		Number of channels within the configured encoder limit.
 *
 * @retval	0	All contexts allocated.
 * @retval	-ENOMEM	Allocation failed; all encoder contexts have been released.
 */
static int sw_codec_g722_enc_init(uint8_t num_ch)
{
	for (int i = 0; i < num_ch; i++) {
		g722_enc_ctx[i] = g722_encoder_new(G722_BITRATE_BPS, G722_DEFAULT);

		if (g722_enc_ctx[i] == NULL) {
			LOG_ERR("Failed to allocate G.722 encoder for channel %d", i);
			sw_codec_g722_enc_uninit_all();
			return -ENOMEM;
		}
	}

	return 0;
}

/**
 * @brief Allocate one independent 64 kbit/s decoder context per channel.
 *
 * @pre No decoder direction is currently initialized.
 * @pre Profile and channel count have been validated by sw_codec_g722_init().
 *
 * @param[in]	num_ch		Number of channels within the configured decoder limit.
 *
 * @retval	0	All contexts allocated.
 * @retval	-ENOMEM	Allocation failed; all decoder contexts have been released.
 */
static int sw_codec_g722_dec_init(uint8_t num_ch)
{
	for (int i = 0; i < num_ch; i++) {
		g722_dec_ctx[i] = g722_decoder_new(G722_BITRATE_BPS, G722_DEFAULT);

		if (g722_dec_ctx[i] == NULL) {
			LOG_ERR("Failed to allocate G.722 decoder for channel %d", i);
			sw_codec_g722_dec_uninit_all();
			return -ENOMEM;
		}
	}

	return 0;
}

int sw_codec_g722_metadata_check(const struct audio_metadata *pcm,
				 const struct audio_metadata *encoded, bool encoder,
				 const struct sw_codec_config *current)
{
	uint8_t pcm_channels = audio_metadata_num_loc_get(pcm);
	uint8_t encoded_channels = audio_metadata_num_loc_get(encoded);
	uint8_t channels_max = encoder ? current->encoder.num_ch : current->decoder.num_ch;
	bool invalid = false;

	/* Both streams use the same fixed 10 ms frame duration. */
	if (pcm->data_len_us != G722_MAX_FRAME_SIZE_US ||
	    encoded->data_len_us != G722_MAX_FRAME_SIZE_US) {
		LOG_ERR("G.722 requires 10 ms frames");
		invalid = true;
	}

	/* Validate encoded frame metadata. */
	if (encoded->sample_rate_hz != G722_SAMPLE_RATE_HZ) {
		LOG_ERR("G.722 encoded frame requires %d Hz", G722_SAMPLE_RATE_HZ);
		invalid = true;
	}

	if (encoded->bytes_per_location != G722_ENC_MONO_FRAME_SIZE) {
		LOG_ERR("G.722 encoded frame requires %d bytes per channel",
			G722_ENC_MONO_FRAME_SIZE);
		invalid = true;
	}

	if (encoder && encoded->bitrate_bps != G722_BITRATE_BPS) {
		LOG_ERR("G.722 encoder requires %d bps", G722_BITRATE_BPS);
		invalid = true;
	}

	/* G.722 uses 16 kHz PCM; 48 kHz is accepted only when SRC is enabled. */
	bool supported_pcm_rate = pcm->sample_rate_hz == G722_SAMPLE_RATE_HZ;

	if (!supported_pcm_rate && IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER)) {
		supported_pcm_rate = pcm->sample_rate_hz == 48000;
	}

	if (!supported_pcm_rate) {
		LOG_ERR("Unsupported G.722 PCM sample rate: %u Hz", pcm->sample_rate_hz);
		invalid = true;
	}

	if (pcm->bits_per_sample != 16) {
		LOG_ERR("G.722 requires 16-bit PCM samples (got %u bits)", pcm->bits_per_sample);
		invalid = true;
	}

	if (pcm->carried_bits_per_sample != 16) {
		LOG_ERR("G.722 requires 16 carried bits per sample (got %u)",
			pcm->carried_bits_per_sample);
		invalid = true;
	}

	/* Frame sizing depends on a supported rate and sample format. */
	if (supported_pcm_rate && pcm->bits_per_sample == 16 &&
	    pcm->carried_bits_per_sample == 16) {
		if (encoder &&
		    pcm->bytes_per_location != pcm->sample_rate_hz / 100 * sizeof(int16_t)) {
			LOG_ERR("Invalid G.722 encoder PCM frame size: %u bytes",
				pcm->bytes_per_location);
			invalid = true;
		}

		if (pcm->sample_rate_hz / 100 * sizeof(int16_t) > PCM_NUM_BYTES_MONO) {
			LOG_ERR("G.722 PCM frame exceeds buffer capacity");
			invalid = true;
		}
	}

	/* Validate channel locations and counts. */
	uint8_t pcm_channels_max =
		encoder ? CONFIG_AUDIO_INPUT_CHANNELS : CONFIG_AUDIO_OUTPUT_CHANNELS;

	if (pcm->locations & ~BT_AUDIO_LOCATION_ANY) {
		LOG_ERR("Unsupported G.722 PCM channel location");
		invalid = true;
	} else if (pcm_channels > pcm_channels_max) {
		LOG_ERR("G.722 PCM channel count exceeds application limit");
		invalid = true;
	}

	if (encoded->locations & ~BT_AUDIO_LOCATION_ANY) {
		LOG_ERR("Unsupported G.722 encoded channel location");
		invalid = true;
	} else if (encoded_channels > channels_max) {
		LOG_ERR("G.722 encoded channel count exceeds initialized channels");
		invalid = true;
	}

	return invalid ? -EINVAL : 0;
}

int sw_codec_g722_decode_validate(size_t payload_size, const struct audio_metadata *encoded,
				  const struct audio_metadata *pcm,
				  const struct sw_codec_config *current)
{
	int ret = sw_codec_g722_metadata_check(pcm, encoded, false, current);

	if (ret != 0) {
		return ret;
	}

	if (payload_size > G722_ENC_MONO_FRAME_SIZE * audio_metadata_num_loc_get(encoded)) {
		LOG_ERR("Oversized G.722 payload");
		return -EINVAL;
	}

	/* Missing bytes may only belong to channels explicitly marked bad. */
	uint32_t locations = encoded->locations ? encoded->locations : 1;
	uint8_t channel = 0;

	for (uint32_t bit = 1; bit != 0; bit <<= 1) {
		if (!(locations & bit)) {
			continue;
		}

		if (!(encoded->bad_data & bit)) {
			if (payload_size < (channel + 1) * G722_ENC_MONO_FRAME_SIZE) {
				LOG_ERR("Truncated valid G.722 channel");
				return -EINVAL;
			}
		}

		channel++;
	}

	return 0;
}

void sw_codec_g722_enc_uninit_all(void)
{
	for (int i = 0; i < ARRAY_SIZE(g722_enc_ctx); i++) {
		if (g722_enc_ctx[i] != NULL) {
			g722_encoder_destroy(g722_enc_ctx[i]);
			g722_enc_ctx[i] = NULL;
		}
	}
}

void sw_codec_g722_dec_uninit_all(void)
{
	for (int i = 0; i < ARRAY_SIZE(g722_dec_ctx); i++) {
		if (g722_dec_ctx[i] != NULL) {
			g722_decoder_destroy(g722_dec_ctx[i]);
			g722_dec_ctx[i] = NULL;
		}
	}
}

int sw_codec_g722_enc_run(void const *const pcm_data, uint32_t pcm_data_size, uint32_t bitrate,
			  uint8_t audio_ch, uint16_t g722_data_buf_size, uint8_t *const g722_data,
			  uint16_t *const g722_data_wr_size)
{
	int ret;
	int num_samples = pcm_data_size / sizeof(int16_t);

	if (audio_ch >= ARRAY_SIZE(g722_enc_ctx)) {
		LOG_ERR("G.722 encoder channel index out of range: %u", audio_ch);
		return -EINVAL;
	}

	if (g722_enc_ctx[audio_ch] == NULL) {
		LOG_ERR("G.722 encoder for channel %d not initialized", audio_ch);
		return -EINVAL;
	}

	if (bitrate != G722_BITRATE_BPS) {
		LOG_ERR("Unsupported G.722 bitrate: %u", bitrate);
		return -EINVAL;
	}

	if (pcm_data_size != G722_PCM_FRAME_BYTES) {
		LOG_ERR("G.722 requires an aligned 160-sample PCM frame, got %u bytes",
			pcm_data_size);
		return -EINVAL;
	}

	if (!IS_ALIGNED((uintptr_t)pcm_data, sizeof(int16_t))) {
		LOG_ERR("G.722 PCM input must be int16-aligned");
		return -EINVAL;
	}

	if (g722_data_buf_size < G722_ENC_MONO_FRAME_SIZE) {
		LOG_ERR("G.722 encoder output buffer too small: %u", g722_data_buf_size);
		return -ENOMEM;
	}

	ret = g722_encode(g722_enc_ctx[audio_ch], (const int16_t *)pcm_data, num_samples,
			  g722_data);
	if (ret != G722_ENC_MONO_FRAME_SIZE) {
		LOG_ERR("Unexpected G.722 encoder output length: %d", ret);
		return -EIO;
	}

	*g722_data_wr_size = ret;

	return 0;
}

int sw_codec_g722_dec_run(uint8_t const *const g722_data, uint16_t g722_data_size,
			  uint16_t pcm_data_buf_size, uint8_t audio_ch, void *const pcm_data,
			  uint16_t *const pcm_data_wr_size, bool bad_frame)
{
	int ret;
	size_t pcm_bytes = G722_PCM_FRAME_BYTES;

	if (audio_ch >= ARRAY_SIZE(g722_dec_ctx)) {
		LOG_ERR("G.722 decoder channel index out of range: %u", audio_ch);
		return -EINVAL;
	}

	if (g722_dec_ctx[audio_ch] == NULL) {
		LOG_ERR("G.722 decoder for channel %d not initialized", audio_ch);
		return -EINVAL;
	}

	if (pcm_bytes > MIN(PCM_NUM_BYTES_MONO, pcm_data_buf_size)) {
		LOG_ERR("G.722 decoder output too large: %zu bytes", pcm_bytes);
		return -ENOMEM;
	}

	if (!IS_ALIGNED((uintptr_t)pcm_data, sizeof(int16_t))) {
		LOG_ERR("G.722 PCM output must be int16-aligned");
		return -ENOMEM;
	}

	if (bad_frame) {
		/* G.722 has no packet loss concealment, output silence */
		memset(pcm_data, 0, pcm_bytes);
		*pcm_data_wr_size = pcm_bytes;
		return 0;
	}

	if (g722_data_size != G722_ENC_MONO_FRAME_SIZE) {
		LOG_ERR("Invalid G.722 payload length: %u", g722_data_size);
		return -EINVAL;
	}

	if (g722_data == NULL) {
		LOG_ERR("G.722 payload pointer is NULL");
		return -EINVAL;
	}

	ret = g722_decode(g722_dec_ctx[audio_ch], g722_data, g722_data_size, (int16_t *)pcm_data);
	if (ret != G722_PCM_FRAME_BYTES / sizeof(int16_t)) {
		LOG_ERR("Unexpected G.722 decoder output length: %d", ret);
		return -EIO;
	}

	/* The library returns samples; the shared codec interface reports bytes. */
	*pcm_data_wr_size = ret * sizeof(int16_t);

	return 0;
}

int sw_codec_g722_init(struct sw_codec_config cfg, struct sw_codec_config *current,
		       struct sample_rate_converter_ctx *encoder_converters,
		       struct sample_rate_converter_ctx *decoder_converters)
{
	int ret;
	bool invalid = false;
	bool already_initialized = false;

	if (!cfg.encoder.enabled && !cfg.decoder.enabled) {
		LOG_ERR("G.722 initialization requires an encoder or decoder direction");
		return -EINVAL;
	}

	if (cfg.encoder.enabled) {
		if (cfg.encoder.num_ch == 0) {
			LOG_ERR("G.722 encoder requires at least one channel");
			invalid = true;
		}

		if (cfg.encoder.num_ch > ARRAY_SIZE(g722_enc_ctx)) {
			LOG_ERR("G.722 encoder channel count exceeds context capacity");
			invalid = true;
		}

		if (cfg.encoder.sample_rate_hz != G722_SAMPLE_RATE_HZ) {
			LOG_ERR("G.722 encoder only supports %d Hz", G722_SAMPLE_RATE_HZ);
			invalid = true;
		}

		if (cfg.encoder.bitrate != G722_BITRATE_BPS) {
			LOG_ERR("G.722 encoder only supports %d bps", G722_BITRATE_BPS);
			invalid = true;
		}
	}

	if (cfg.decoder.enabled) {
		if (cfg.decoder.num_ch == 0) {
			LOG_ERR("G.722 decoder requires at least one channel");
			invalid = true;
		}

		if (cfg.decoder.num_ch > ARRAY_SIZE(g722_dec_ctx)) {
			LOG_ERR("G.722 decoder channel count exceeds context capacity");
			invalid = true;
		}

		if (cfg.decoder.sample_rate_hz != G722_SAMPLE_RATE_HZ) {
			LOG_ERR("G.722 decoder only supports %d Hz", G722_SAMPLE_RATE_HZ);
			invalid = true;
		}
	}

	if (cfg.encoder.enabled && current->encoder.enabled) {
		LOG_WRN("G.722 encoder is already initialized");
		already_initialized = true;
	}

	if (cfg.decoder.enabled && current->decoder.enabled) {
		LOG_WRN("G.722 decoder is already initialized");
		already_initialized = true;
	}

	if (invalid) {
		return -EINVAL;
	}

	if (already_initialized) {
		return -EALREADY;
	}

	if (cfg.encoder.enabled) {
		ret = sw_codec_g722_enc_init(cfg.encoder.num_ch);
		if (ret != 0) {
			return ret;
		}
	}

	if (cfg.decoder.enabled) {
		ret = sw_codec_g722_dec_init(cfg.decoder.num_ch);
		if (ret != 0) {
			goto rollback;
		}
	}

	if (IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER)) {
		if (cfg.encoder.enabled) {
			for (int i = 0; i < cfg.encoder.num_ch; i++) {
				ret = sample_rate_converter_open(&encoder_converters[i]);
				if (ret != 0) {
					LOG_ERR("G.722 encoder converter init failed: %d", ret);
					goto rollback;
				}
			}
		}

		if (cfg.decoder.enabled) {
			for (int i = 0; i < cfg.decoder.num_ch; i++) {
				ret = sample_rate_converter_open(&decoder_converters[i]);
				if (ret != 0) {
					LOG_ERR("G.722 decoder converter init failed: %d", ret);
					goto rollback;
				}
			}
		}
	}

	current->sw_codec = SW_CODEC_G722;

	if (cfg.encoder.enabled) {
		current->encoder = cfg.encoder;
	}

	if (cfg.decoder.enabled) {
		current->decoder = cfg.decoder;
	}

	current->initialized = true;

	return 0;

rollback:
	/* Only requested directions can own new contexts; leave the opposite one intact. */
	if (cfg.encoder.enabled) {
		sw_codec_g722_enc_uninit_all();
	}

	if (cfg.decoder.enabled) {
		sw_codec_g722_dec_uninit_all();
	}

	return ret;
}
