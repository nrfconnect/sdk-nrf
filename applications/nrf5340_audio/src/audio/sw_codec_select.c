/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sw_codec_select.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <errno.h>
#include <pcm_stream_channel_modifier.h>
#include <sample_rate_converter.h>

#if (CONFIG_SW_CODEC_LC3)
#include "sw_codec_lc3.h"
#endif /* (CONFIG_SW_CODEC_LC3) */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sw_codec_select, CONFIG_SW_CODEC_SELECT_LOG_LEVEL);

#define BLK_PERIOD_US 10000

/* Number of audio blocks given a duration */
#define NUM_BLKS(d) ((d) / BLK_PERIOD_US)

static struct sw_codec_config m_config;

static struct sample_rate_converter_ctx encoder_converters[CONFIG_AUDIO_ENCODE_CHANNELS_MAX];
static struct sample_rate_converter_ctx decoder_converters[CONFIG_AUDIO_DECODE_CHANNELS_MAX];

/**
 * @brief	Converts the sample rate of the uncompressed audio stream if needed.
 *
 * @details	Two buffers must be made available for the function: the input_data buffer that
 *		contains the samples for the audio stream, and the conversion buffer that will be
 *		used to store the converted audio stream. data_ptr will point to conversion_buffer
 *		if a conversion took place; otherwise, it will point to input_data.
 *
 * @param[in]	ctx			Sample rate converter context.
 * @param[in]	input_sample_rate	Input sample rate.
 * @param[in]	output_sample_rate	Output sample rate.
 * @param[in]	input_data		Data coming in. Buffer is assumed to be of size
 *					PCM_NUM_BYTES_MONO.
 * @param[in]	input_data_size		Size of input data.
 * @param[in]	conversion_buffer	Buffer to perform sample rate conversion. Must be of size
 *					PCM_NUM_BYTES_MONO.
 * @param[out]	data_ptr		Pointer to the data to be used from this point on.
 *					Will point to either @p input_data or @p conversion_buffer.
 * @param[out]	output_size		Number of bytes out.
 *
 * @retval	-ENOTSUP	Sample rates are not equal, and the sample rate conversion has not
 *been enabled in the application.
 * @retval	0		Success.
 */
static int sw_codec_sample_rate_convert(struct sample_rate_converter_ctx *ctx,
					uint32_t input_sample_rate, uint32_t output_sample_rate,
					char *input_data, size_t input_data_size,
					char *conversion_buffer, char **data_ptr,
					size_t *output_size)
{
	int ret;

	if (input_sample_rate == output_sample_rate) {
		LOG_DBG("Sample rates are the same.");
		*data_ptr = input_data;
		*output_size = input_data_size;
	} else if (IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER)) {
		ret = sample_rate_converter_process(ctx, SAMPLE_RATE_FILTER_SIMPLE, input_data,
						    input_data_size, input_sample_rate,
						    conversion_buffer, PCM_NUM_BYTES_MONO,
						    output_size, output_sample_rate);
		if (ret) {
			LOG_ERR("Failed to convert sample rate: %d", ret);
			return ret;
		}

		*data_ptr = conversion_buffer;
	} else {
		LOG_ERR("Sample rates are not equal, and sample rate conversion has not been "
			"enabled in the application.");
		return -ENOTSUP;
	}

	return 0;
}

bool sw_codec_is_initialized(void)
{
	return m_config.initialized;
}

int sw_codec_encode(struct net_buf *audio_frame_in, struct net_buf *audio_frame_out)
{
	if (!m_config.encoder.enabled) {
		LOG_ERR("Encoder has not been initialized");
		return -ENXIO;
	}

	if (audio_frame_in == NULL || audio_frame_out == NULL) {
		LOG_ERR("LC3 encoder input parameter error");
		return -EINVAL;
	}

	int ret;
	struct audio_metadata *meta_in = net_buf_user_data(audio_frame_in);
	struct audio_metadata *meta_out = net_buf_user_data(audio_frame_out);

	switch (m_config.sw_codec) {
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		struct lc3_encoder_context *ctx = &m_config.encoder.lc3_ctx;
		size_t session_in_size;
		uint8_t *data_in = (uint8_t *)audio_frame_in->data;
		uint8_t *coded_out;
		uint8_t temp_pcm[PCM_NUM_BYTES_MONO];
		uint8_t resamp_pcm_temp[PCM_NUM_BYTES_MONO];
		uint8_t chans_in, chans_out;
		char *resamp_pcm;
		size_t resamp_pcm_size;
		size_t data_in_size = 0;
		uint16_t encoded_bytes_written, out_remaining;

		LOG_DBG("LC3 encoder module");

		if (meta_in->data_coding != PCM || meta_out->data_coding != LC3) {
			LOG_ERR("LC3 encoder module has incorrect input or output data type: in = "
				"%d  out = %d",
				meta_in->data_coding, meta_out->data_coding);
			return -EINVAL;
		}

		chans_in = sw_codec_metadata_num_ch_get(meta_in);
		chans_out = sw_codec_metadata_num_ch_get(meta_out);

		if (audio_frame_in->len >= ctx->config.sample_frame_bytes * chans_in) {
			session_in_size = audio_frame_in->len;
		} else {
			LOG_WRN("Input buffer too small: %d (>=%d)", audio_frame_in->len,
				(ctx->config.sample_frame_bytes * chans_in));
			session_in_size = 0;
		}

		if (audio_frame_out->size < ctx->config.coded_frame_bytes * chans_out) {
			LOG_ERR("Output buffer too small: %d (>=%d)", audio_frame_out->size,
				(ctx->config.coded_frame_bytes * chans_out));
			return -EINVAL;
		}

		coded_out = (uint8_t *)audio_frame_out->data;
		out_remaining = audio_frame_out->size;

		/* Should be able to encode only the channel(s) of interest here.
		 * These will be put in the first channel or channels and the location
		 * will indicate which channel(s) they are. Prior to playout (I2S or TDM)
		 * all other channels can be zeroed.
		 */
		for (uint8_t i = 0; i < chans_out; i++) {
			if (ctx->config.interleaved && chans_in > 1) {
				ret = pscm_uninterleave(audio_frame_in->data, audio_frame_in->size,
							chans_in, i,
							ctx->config.carried_bits_per_sample,
							temp_pcm, sizeof(temp_pcm));
				if (ret) {
					LOG_DBG("Failed to uninterleave input");
					return ret;
				}

				data_in = temp_pcm;
				data_in_size = sizeof(temp_pcm);

				LOG_DBG("Completed encoder PCM input uninterleaving for ch: %d", i);
			} else {
				data_in = (uint8_t *)audio_frame_in->data +
					  (meta_in->bytes_per_location * i);
				data_in_size =
					audio_frame_in->len - (meta_in->bytes_per_location * i);
			}

			ret = sw_codec_sample_rate_convert(
				&encoder_converters[i], meta_in->sample_rate_hz,
				meta_out->sample_rate_hz, data_in, data_in_size, resamp_pcm_temp,
				&resamp_pcm, &resamp_pcm_size);
			if (ret) {
				LOG_ERR("Sample rate conversion failed for channel "
					"%d: %d",
					i, ret);
				return ret;
			}

			ret = sw_codec_lc3_enc_run(resamp_pcm, resamp_pcm_size,
						   ctx->config.bitrate_bps, i, session_in_size,
						   coded_out, &encoded_bytes_written);
			if (ret) {
				LOG_DBG("Error in ENCODER, ret: %d", ret);
				return ret;
			}

			coded_out += encoded_bytes_written;
			out_remaining -= encoded_bytes_written;

			LOG_DBG("Completed LC3 encode of ch: %d", i);
		}

		meta_out->bytes_per_location = encoded_bytes_written;
		net_buf_add(audio_frame_out, audio_frame_out->size - out_remaining);
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return -ENODEV;
	}

	return 0;
}

int sw_codec_decode(struct net_buf const *const audio_frame_in,
		    struct net_buf *const audio_frame_out)
{
	if (!m_config.decoder.enabled) {
		LOG_ERR("Decoder has not been initialized");
		return -ENXIO;
	}

	int ret;
	struct audio_metadata *meta_in = net_buf_user_data(audio_frame_in);
	struct audio_metadata *meta_out = net_buf_user_data(audio_frame_out);

	switch (m_config.sw_codec) {
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		struct lc3_decoder_context *ctx = &m_config.decoder.lc3_ctx;
		size_t session_in_size;
		uint16_t bytesWritten;
		uint8_t *data_in;
		uint8_t *data_out;
		uint8_t temp_pcm[PCM_NUM_BYTES_MONO];
		uint8_t chans_in, chans_out;
		uint8_t *resamp_pcm;
		size_t resamp_pcm_size = 0;
		uint32_t bad_data_mask;

		LOG_DBG("LC3 decoder module");

		if (meta_in->data_coding != LC3 || meta_out->data_coding != PCM) {
			LOG_DBG("LC3 decoder module has incorrect input or output data type: in = "
				"%d  out = %d",
				meta_in->data_coding, meta_out->data_coding);
			return -EINVAL;
		}

		meta_out->bits_per_sample = ctx->config.bits_per_sample;
		meta_out->carried_bits_per_sample = ctx->config.carried_bits_per_sample;
		meta_out->locations = ctx->config.locations;

		chans_in = sw_codec_metadata_num_ch_get(meta_in);
		chans_out = sw_codec_metadata_num_ch_get(meta_out);

		if (chans_out <= CONFIG_AUDIO_OUTPUT_CHANNELS) {
			memset(audio_frame_out->data, 0, audio_frame_out->size);
		} else {
			LOG_WRN("Output channels (%d) exceeds maximum (%d)", chans_out,
				CONFIG_AUDIO_OUTPUT_CHANNELS);
		}

		chans_out = CONFIG_AUDIO_OUTPUT_CHANNELS;

		if (meta_in->bytes_per_location <= ctx->config.coded_frame_bytes) {
			session_in_size = meta_in->bytes_per_location;
		} else {
			session_in_size = 0;
		}

		if (audio_frame_out->size < ctx->config.sample_frame_bytes * chans_out) {
			LOG_ERR("Output buffer too small: %d (<=%d)", audio_frame_out->size,
				(ctx->config.sample_frame_bytes * chans_out));
			return -EINVAL;
		}

		if (ctx->config.interleaved && chans_out > 1) {
			data_out = temp_pcm;
		} else {
			data_out = (uint8_t *)audio_frame_out->data;
		}

		/* Should be able to decode only the channel(s) of interest here.
		 * These will be put in the first channel or channels and the location
		 * will indicate which channel(s) they are. Prior to playout (I2S or TDM)
		 * all other channels can be zeroed.
		 */
		for (uint8_t i = 0; i < chans_in; i++) {
			data_in = (uint8_t *)audio_frame_in->data + (session_in_size * i);
			bad_data_mask = BIT(i);

			ret = sw_codec_lc3_dec_run(data_in, session_in_size, audio_frame_out->size,
						   i, data_out, &bytesWritten,
						   (meta_in->bad_data & bad_data_mask));
			if (ret) {
				LOG_ERR("Decode for channel %d failed: %d", i, ret);
				return ret;
			}

			ret = sw_codec_sample_rate_convert(
				&decoder_converters[i], meta_in->sample_rate_hz,
				meta_out->sample_rate_hz, data_out, bytesWritten, &temp_pcm[0],
				(char **)&resamp_pcm, &resamp_pcm_size);
			if (ret) {
				LOG_ERR("Sample rate conversion failed for mono: %d", ret);
				return ret;
			}

			if (ctx->config.interleaved && chans_out > 1) {
				ret = pscm_interleave(resamp_pcm, resamp_pcm_size, i,
						      meta_out->carried_bits_per_sample,
						      audio_frame_out->data, audio_frame_out->size,
						      chans_out);

				if (ret) {
					LOG_ERR("Failed to interleave output: %d", ret);
					return ret;
				}

				LOG_DBG("Completed decoders PCM interleaving for ch: %d", i);
			} else {
				data_out += resamp_pcm_size;
			}

			LOG_DBG("Completed LC3 decode of ch: %d", i);
		}

		meta_out->bytes_per_location = resamp_pcm_size;

		net_buf_add(audio_frame_out, resamp_pcm_size * chans_out);
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return -ENODEV;
	}

	LOG_DBG("Completed LC3 decoding");
	return 0;
}

int sw_codec_uninit(struct sw_codec_config sw_codec_cfg)
{
	int ret;

	if (m_config.sw_codec != sw_codec_cfg.sw_codec) {
		LOG_ERR("Trying to uninit a codec that is not first initialized");
		return -ENODEV;
	}
	switch (m_config.sw_codec) {
	case SW_CODEC_LC3:
#if (CONFIG_SW_CODEC_LC3)
		if (sw_codec_cfg.encoder.enabled) {
			if (!m_config.encoder.enabled) {
				LOG_ERR("Trying to uninit encoder, it has not been "
					"initialized");
				return -EALREADY;
			}
			ret = sw_codec_lc3_enc_uninit_all();
			if (ret) {
				return ret;
			}
			m_config.encoder.enabled = false;
		}

		if (sw_codec_cfg.decoder.enabled) {
			if (!m_config.decoder.enabled) {
				LOG_WRN("Trying to uninit decoder, it has not been "
					"initialized");
				return -EALREADY;
			}

			ret = sw_codec_lc3_dec_uninit_all();
			if (ret) {
				return ret;
			}
			m_config.decoder.enabled = false;
		}
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return false;
	}

	m_config.initialized = false;

	return 0;
}

int sw_codec_init(struct sw_codec_config sw_codec_cfg)
{
	int ret;

	switch (sw_codec_cfg.sw_codec) {
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		if (m_config.sw_codec != SW_CODEC_LC3) {
			/* Check if LC3 is already initialized */
			ret = sw_codec_lc3_init(NULL, NULL, CONFIG_AUDIO_FRAME_DURATION_US);
			if (ret) {
				return ret;
			}
		}

		if (sw_codec_cfg.encoder.enabled) {
			if (m_config.encoder.enabled) {
				LOG_WRN("The LC3 encoder is already initialized");
				return -EALREADY;
			}
			uint16_t pcm_bytes_req_enc;

			LOG_DBG("Encode: %dHz %dbits %dus %dbps %d channel(s)",
				sw_codec_cfg.encoder.sample_rate_hz, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US, sw_codec_cfg.encoder.bitrate,
				sw_codec_cfg.encoder.num_ch);

			ret = sw_codec_lc3_enc_init(
				sw_codec_cfg.encoder.sample_rate_hz, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US, sw_codec_cfg.encoder.bitrate,
				sw_codec_cfg.encoder.num_ch, &pcm_bytes_req_enc);
			if (ret) {
				return ret;
			}

			struct lc3_configuration *config = &sw_codec_cfg.encoder.lc3_ctx.config;

			ret = sw_codec_lc3_dec_size_get(
				sw_codec_cfg.encoder.sample_rate_hz, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US, &config->sample_frame_bytes);
			if (ret) {
				return ret;
			}

			config->bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			config->carried_bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_OCTETS * 8;
			config->interleaved = true;
			config->bitrate_bps = sw_codec_cfg.encoder.bitrate;
			config->coded_frame_bytes = ENC_MAX_FRAME_SIZE;
			config->locations = sw_codec_cfg.decoder.audio_ch;
		}

		if (sw_codec_cfg.decoder.enabled) {
			if (m_config.decoder.enabled) {
				LOG_WRN("The LC3 decoder is already initialized");
				return -EALREADY;
			}

			LOG_DBG("Decode: %dHz %dbits %dus %d channel(s)",
				sw_codec_cfg.decoder.sample_rate_hz, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US, sw_codec_cfg.decoder.num_ch);

			ret = sw_codec_lc3_dec_init(
				sw_codec_cfg.decoder.sample_rate_hz, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US, sw_codec_cfg.decoder.num_ch);
			if (ret) {
				return ret;
			}

			struct lc3_configuration *config = &sw_codec_cfg.decoder.lc3_ctx.config;

			ret = sw_codec_lc3_enc_size_get(
				sw_codec_cfg.decoder.sample_rate_hz, CONFIG_LC3_BITRATE,
				CONFIG_AUDIO_FRAME_DURATION_US, &config->coded_frame_bytes);
			if (ret) {
				return ret;
			}

			config->bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			config->carried_bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_OCTETS * 8;
			config->interleaved = true;
			config->bitrate_bps = CONFIG_LC3_BITRATE;
			config->sample_frame_bytes = PCM_NUM_BYTES_MONO;
			config->locations = sw_codec_cfg.decoder.audio_ch;
		}
		break;
#else
		LOG_ERR("LC3 is not compiled in, please open menuconfig and select "
			"LC3");
		return -ENODEV;
#endif /* (CONFIG_SW_CODEC_LC3) */
	}

	default:
		LOG_ERR("Unsupported codec: %d", sw_codec_cfg.sw_codec);
		return false;
	}

	if (sw_codec_cfg.encoder.enabled && IS_ENABLED(SAMPLE_RATE_CONVERTER)) {
		for (int i = 0; i < sw_codec_cfg.encoder.channel_mode; i++) {
			ret = sample_rate_converter_open(&encoder_converters[i]);
			if (ret) {
				LOG_ERR("Failed to initialize the sample rate converter for "
					"encoding channel %d: %d",
					i, ret);
				return ret;
			}
		}
	}

	if (sw_codec_cfg.decoder.enabled && IS_ENABLED(SAMPLE_RATE_CONVERTER)) {
		for (int i = 0; i < sw_codec_cfg.decoder.channel_mode; i++) {
			ret = sample_rate_converter_open(&decoder_converters[i]);
			if (ret) {
				LOG_ERR("Failed to initialize the sample rate converter for "
					"decoding channel %d: %d",
					i, ret);
				return ret;
			}
		}
	}

	m_config = sw_codec_cfg;
	m_config.initialized = true;

	return 0;
}
