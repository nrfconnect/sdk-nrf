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

#define CONFIG_AUDIO_BIT_RATE_MAX 96000
#define BLK_PERIOD_US		  10000

/* Number of audio blocks given a duration */
#define NUM_BLKS(d) ((d) / BLK_PERIOD_US)

static struct sw_codec_config m_config;

static struct sample_rate_converter_ctx encoder_converters[AUDIO_CH_NUM];
static struct sample_rate_converter_ctx decoder_converters[AUDIO_CH_NUM];

#if (CONFIG_SW_CODEC_LC3)
static int sw_codec_config_set(struct lc3_decoder_context *ctx, uint32_t bitrate_bps_max,
			       uint16_t pcm_sample_rate, uint8_t pcm_bit_depth,
			       uint16_t framesize_us, uint32_t locations, uint8_t carried_octets,
			       bool interleaved)
{
	int ret;
	uint8_t num_channels;
	uint32_t mask = locations;

	if (mask == 0) {
		num_channels = 1;
	}
	while (mask) {
		num_channels += mask & 1;
		mask >>= 1;
	}

	ret = sw_codec_lc3_get_coded_size(pcm_sample_rate, bitrate_bps_max, framesize_us,
					  &ctx->coded_bytes_req);
	if (ret) {
		LOG_ERR("Required coded bytes to LC3 instance is zero");
		return -EPERM;
	}

	ctx->sample_frame_bytes =
		((framesize_us * pcm_sample_rate) / LC3_DECODER_US_IN_A_SECOND) * carried_octets;
	ctx->plc_count = 0;

	ctx->config.sample_rate_hz = pcm_sample_rate;
	ctx->config.bits_per_sample = pcm_bit_depth;
	ctx->config.carried_bits_per_sample = carried_octets;
	ctx->config.data_len_us = framesize_us;
	ctx->config.interleaved = interleaved;
	ctx->config.locations = locations;
	ctx->config.bitrate_bps_max = bitrate_bps_max;

	return 0;
}
#endif /* (CONFIG_SW_CODEC_LC3) */

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

int sw_codec_encode(struct net_buf *audio_frame_pcm, struct net_buf *audio_frame_lc3)
{
	int ret;

	/* Temp storage for split stereo PCM signal */
	char pcm_data_mono_system_sample_rate[AUDIO_CH_NUM][PCM_NUM_BYTES_MONO] = {0};

	char pcm_data_mono_converted_buf[AUDIO_CH_NUM][PCM_NUM_BYTES_MONO] = {0};

	size_t pcm_block_size_mono_system_sample_rate;
	size_t pcm_block_size_mono;

	struct audio_metadata *meta_lc3 = net_buf_user_data(audio_frame_lc3);

	if (!m_config.encoder.enabled) {
		LOG_ERR("Encoder has not been initialized");
		return -ENXIO;
	}

	switch (m_config.sw_codec) {
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		uint16_t encoded_bytes_written;
		char *pcm_data_mono_ptrs[m_config.encoder.channel_mode];

		/* Since LC3 is a single channel codec, we must split the
		 * stereo PCM stream
		 */
		ret = pscm_two_channel_split(audio_frame_pcm->data, audio_frame_pcm->len,
					     CONFIG_AUDIO_BIT_DEPTH_BITS,
					     pcm_data_mono_system_sample_rate[AUDIO_CH_L],
					     pcm_data_mono_system_sample_rate[AUDIO_CH_R],
					     &pcm_block_size_mono_system_sample_rate);
		if (ret) {
			return ret;
		}

		for (int i = 0; i < m_config.encoder.channel_mode; ++i) {
			ret = sw_codec_sample_rate_convert(
				&encoder_converters[i], CONFIG_AUDIO_SAMPLE_RATE_HZ,
				m_config.encoder.sample_rate_hz,
				pcm_data_mono_system_sample_rate[i],
				pcm_block_size_mono_system_sample_rate,
				pcm_data_mono_converted_buf[i], &pcm_data_mono_ptrs[i],
				&pcm_block_size_mono);
			if (ret) {
				LOG_ERR("Sample rate conversion failed for channel %d: %d", i, ret);
				return ret;
			}
		}

		switch (m_config.encoder.channel_mode) {
		case SW_CODEC_MONO: {
			ret = sw_codec_lc3_enc_run(pcm_data_mono_ptrs[AUDIO_CH_L],
						   pcm_block_size_mono, LC3_USE_BITRATE_FROM_INIT,
						   0, audio_frame_lc3->len, audio_frame_lc3->data,
						   &encoded_bytes_written);
			if (ret) {
				return ret;
			}
			meta_lc3->locations = BT_AUDIO_LOCATION_FRONT_LEFT;
			break;
		}
		case SW_CODEC_STEREO: {
			ret = sw_codec_lc3_enc_run(pcm_data_mono_ptrs[AUDIO_CH_L],
						   pcm_block_size_mono, LC3_USE_BITRATE_FROM_INIT,
						   AUDIO_CH_L, audio_frame_lc3->len,
						   audio_frame_lc3->data, &encoded_bytes_written);
			if (ret) {
				return ret;
			}

			ret = sw_codec_lc3_enc_run(
				pcm_data_mono_ptrs[AUDIO_CH_R], pcm_block_size_mono,
				LC3_USE_BITRATE_FROM_INIT, AUDIO_CH_R,
				audio_frame_lc3->len - encoded_bytes_written,
				(uint8_t *)audio_frame_lc3->data + encoded_bytes_written,
				&encoded_bytes_written);
			if (ret) {
				return ret;
			}
			encoded_bytes_written += encoded_bytes_written;
			break;
		}
		default:
			LOG_ERR("Unsupported channel mode for encoder: %d",
				m_config.encoder.channel_mode);
			return -ENODEV;
		}

		meta_lc3->data_coding = LC3;

		net_buf_remove_mem(audio_frame_lc3, audio_frame_lc3->size - encoded_bytes_written);

		net_buf_unref(audio_frame_pcm);
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return -ENODEV;
	}

	return 0;
}

static void audio_buf_zero(struct net_buf const *const audio_frame)
{
	memset(audio_frame->data, 0, audio_frame->size);
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
		struct lc3_decoder_context *ctx = &m_config.decoder.context;
		LC3BFI_t frame_status;
		uint16_t plc_counter = 0;
		size_t session_in_size;
		uint16_t bytesWritten;
		uint8_t *data_in;
		uint8_t *data_out;
		uint8_t temp_pcm[PCM_NUM_BYTES_MONO];
		uint8_t chans_in, chans_out;
		uint8_t *resamp_pcm;
		size_t resamp_pcm_size = 0;

		LOG_DBG("LC3 decoder module");

		if (meta_in->data_coding != LC3 || meta_out->data_coding != PCM) {
			LOG_DBG("LC3 decoder module has incorrect input or output data type: in = "
				"%d  out = %d",
				meta_in->data_coding, meta_out->data_coding);
			return -EINVAL;
		}

		if (meta_in->bad_data) {
			frame_status = BadFrame;
		} else {
			frame_status = GoodFrame;
			ctx->plc_count = 0;
		}

		chans_in = sw_codec_metadata_num_ch_get(meta_in);

		meta_out->locations = ctx->config.locations;
		meta_out->bits_per_sample = ctx->config.bits_per_sample;
		meta_out->carried_bits_per_sample = ctx->config.carried_bits_per_sample;

		if (audio_frame_in->len) {
			session_in_size = audio_frame_in->len / chans_in;
			if (session_in_size < ctx->coded_bytes_req) {
				audio_buf_zero(audio_frame_out);

				LOG_ERR("Too few coded bytes %d to decode. Bytes required input "
					"framesize is %d",
					session_in_size, ctx->coded_bytes_req);
				return -EINVAL;
			}
		} else {
			session_in_size = 0;
		}

		chans_out = sw_codec_metadata_num_ch_get(meta_out);

		if (audio_frame_out->len < ctx->sample_frame_bytes * chans_out) {
			LOG_ERR("Output buffer too small. Bytes required %d, output buffer "
				"is %d",
				(ctx->sample_frame_bytes * chans_out), audio_frame_out->len);
			return -EINVAL;
		}

		if (ctx->config.interleaved) {
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

			ret = sw_codec_lc3_dec_run(data_in, session_in_size,
						   ctx->sample_frame_bytes, i, data_out,
						   &bytesWritten, meta_in->bad_data);
			if (ret) {
				LOG_ERR("Decode for channel %d failed: %d", i, ret);
				return ret;
			}

			ret = sw_codec_sample_rate_convert(
				&decoder_converters[i], m_config.decoder.sample_rate_hz,
				CONFIG_AUDIO_SAMPLE_RATE_HZ, data_out, bytesWritten, &temp_pcm[0],
				(char **)&resamp_pcm, &resamp_pcm_size);
			if (ret) {
				LOG_ERR("Sample rate conversion failed for mono: %d", ret);
				return ret;
			}

			if (ctx->config.interleaved) {
				ret = pscm_interleave(
					resamp_pcm, resamp_pcm_size, i, ctx->config.bits_per_sample,
					audio_frame_out->data, audio_frame_out->size, chans_out);

				if (ret) {
					LOG_DBG("Failed to interleave output");
					return ret;
				}

				LOG_DBG("Completed decoders PCM interleaving for ch: %d", i);
			} else {
				data_out += bytesWritten;
			}

			LOG_DBG("Completed LC3 decode of ch: %d", i);
		}

		ctx->plc_count = plc_counter;

		net_buf_remove_mem(audio_frame_out,
				   audio_frame_out->size - (ctx->sample_frame_bytes * chans_out));

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

			ret = sw_codec_config_set(
				&sw_codec_cfg.decoder.context, CONFIG_AUDIO_BIT_RATE_MAX,
				sw_codec_cfg.decoder.sample_rate_hz, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US,
				BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
				CONFIG_AUDIO_BIT_DEPTH_OCTETS, true);
			if (ret) {
				return ret;
			}
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
