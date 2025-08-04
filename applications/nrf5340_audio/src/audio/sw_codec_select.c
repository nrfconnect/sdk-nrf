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

/* Used for debugging, inserts 0 instead of packet loss concealment */
#define SW_CODEC_OVERRIDE_PLC false

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

int sw_codec_encode(struct net_buf *audio_frame)
{
	int ret;

	/* Temp storage for PCM */
	char pcm_data_mono_system_sample_rate
		[CONFIG_AUDIO_ENCODE_CHANNELS_MAX][PCM_NUM_BYTES_MONO] = {0};
	/* Make sure we have enough space for two frames (stereo) */
	uint8_t m_encoded_data[ENC_MAX_FRAME_SIZE * CONFIG_AUDIO_ENCODE_CHANNELS_MAX];

	char pcm_data_mono_converted_buf[CONFIG_AUDIO_ENCODE_CHANNELS_MAX]
		[PCM_NUM_BYTES_MONO] = {0};

	size_t pcm_block_size_mono_system_sample_rate;
	size_t pcm_block_size_mono;

	struct audio_metadata *meta = net_buf_user_data(audio_frame);

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
		ret = pscm_two_channel_split(audio_frame->data, audio_frame->len,
					     CONFIG_AUDIO_BIT_DEPTH_BITS,
					     pcm_data_mono_system_sample_rate[0],
					     pcm_data_mono_system_sample_rate[1],
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
			ret = sw_codec_lc3_enc_run(pcm_data_mono_ptrs[0],
						   pcm_block_size_mono, LC3_USE_BITRATE_FROM_INIT,
						   0, sizeof(m_encoded_data), m_encoded_data,
						   &encoded_bytes_written);
			if (ret) {
				return ret;
			}
			meta->locations = BT_AUDIO_LOCATION_FRONT_LEFT;
			break;
		}
		case SW_CODEC_STEREO: {
			ret = sw_codec_lc3_enc_run(pcm_data_mono_ptrs[0],
						   pcm_block_size_mono, LC3_USE_BITRATE_FROM_INIT,
						   0, sizeof(m_encoded_data),
						   m_encoded_data, &encoded_bytes_written);
			if (ret) {
				return ret;
			}

			ret = sw_codec_lc3_enc_run(
				pcm_data_mono_ptrs[1], pcm_block_size_mono,
				LC3_USE_BITRATE_FROM_INIT, 1,
				sizeof(m_encoded_data) - encoded_bytes_written,
				m_encoded_data + encoded_bytes_written, &encoded_bytes_written);
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

		meta->data_coding = LC3;

		net_buf_remove_mem(audio_frame, audio_frame->len);
		net_buf_add_mem(audio_frame, m_encoded_data, encoded_bytes_written);
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return -ENODEV;
	}

	return 0;
}

int sw_codec_decode(struct net_buf const *const audio_frame, void **decoded_data,
		    size_t *decoded_size)
{
	if (!m_config.decoder.enabled) {
		LOG_ERR("Decoder has not been initialized");
		return -ENXIO;
	}

	int ret;

	static char pcm_data_stereo[PCM_NUM_BYTES_STEREO];

	char decoded_data_mono
		[CONFIG_AUDIO_DECODE_CHANNELS_MAX][PCM_NUM_BYTES_MONO] = {0};
	char decoded_data_mono_system_sample_rate
		[CONFIG_AUDIO_DECODE_CHANNELS_MAX][PCM_NUM_BYTES_MONO] = {0};

	size_t pcm_size_stereo = 0;
	size_t pcm_size_mono = 0;
	size_t decoded_data_size = 0;

	struct audio_metadata *meta = net_buf_user_data(audio_frame);

	switch (m_config.sw_codec) {
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		char *pcm_in_data_ptrs[m_config.decoder.channel_mode];

		switch (m_config.decoder.channel_mode) {
		case SW_CODEC_MONO: {
			if (meta->bad_data && SW_CODEC_OVERRIDE_PLC) {
				memset(decoded_data_mono[0], 0, PCM_NUM_BYTES_MONO);
				decoded_data_size = PCM_NUM_BYTES_MONO;
			} else {
				ret = sw_codec_lc3_dec_run(
					audio_frame->data, audio_frame->len, LC3_PCM_NUM_BYTES_MONO,
					0, decoded_data_mono[0],
					(uint16_t *)&decoded_data_size, meta->bad_data);
				if (ret) {
					return ret;
				}

				ret = sw_codec_sample_rate_convert(
					&decoder_converters[0],
					m_config.decoder.sample_rate_hz,
					CONFIG_AUDIO_SAMPLE_RATE_HZ, decoded_data_mono[0],
					decoded_data_size,
					decoded_data_mono_system_sample_rate[0],
					&pcm_in_data_ptrs[0], &pcm_size_mono);
				if (ret) {
					LOG_ERR("Sample rate conversion failed for mono: %d", ret);
					return ret;
				}
			}

			if (IS_ENABLED(CONFIG_STREAM_BIDIRECTIONAL) &&
			    (CONFIG_AUDIO_DEV == GATEWAY)) {
				/* If we are receieving mono audio on the gateway, we send it out
				 * as stereo, so we need to pad the mono data to stereo.
				 */
				ret = pscm_copy_pad(pcm_in_data_ptrs[0], pcm_size_mono,
						    CONFIG_AUDIO_BIT_DEPTH_BITS, pcm_data_stereo,
						    &pcm_size_stereo);
				if (ret) {
					LOG_ERR("Failed to copy pad mono to stereo: %d", ret);
					return ret;
				}
			} else {
				/* For now, i2s is only stereo, so in order to send
				 * just one channel, we need to insert 0 for the
				 * other channel
				 */
				ret = pscm_zero_pad(pcm_in_data_ptrs[0], pcm_size_mono,
						    m_config.decoder.audio_ch,
						    CONFIG_AUDIO_BIT_DEPTH_BITS, pcm_data_stereo,
						    &pcm_size_stereo);
				if (ret) {
					return ret;
				}
			}
			break;
		}
		case SW_CODEC_STEREO: {

			if (meta->bad_data && SW_CODEC_OVERRIDE_PLC) {
				memset(decoded_data_mono[0], 0, PCM_NUM_BYTES_MONO);
				memset(decoded_data_mono[1], 0, PCM_NUM_BYTES_MONO);
				decoded_data_size = PCM_NUM_BYTES_MONO;
			} else {
				/* Decode left channel */
				ret = sw_codec_lc3_dec_run(
					audio_frame->data, (audio_frame->len / 2),
					LC3_PCM_NUM_BYTES_MONO, 0,
					decoded_data_mono[0],
					(uint16_t *)&decoded_data_size, meta->bad_data);
				if (ret) {
					return ret;
				}

				/* Decode right channel */
				ret = sw_codec_lc3_dec_run(
					(audio_frame->data + (audio_frame->len / 2)),
					(audio_frame->len / 2), LC3_PCM_NUM_BYTES_MONO, 1,
					decoded_data_mono[1],
					(uint16_t *)&decoded_data_size, meta->bad_data);
				if (ret) {
					return ret;
				}

				for (int i = 0; i < m_config.decoder.channel_mode; ++i) {
					ret = sw_codec_sample_rate_convert(
						&decoder_converters[i],
						m_config.decoder.sample_rate_hz,
						CONFIG_AUDIO_SAMPLE_RATE_HZ, decoded_data_mono[i],
						decoded_data_size,
						decoded_data_mono_system_sample_rate[i],
						&pcm_in_data_ptrs[i], &pcm_size_mono);
					if (ret) {
						LOG_ERR("Sample rate conversion failed for channel "
							"%d : %d",
							i, ret);
						return ret;
					}
				}
			}

			ret = pscm_combine(pcm_in_data_ptrs[0],
					   pcm_in_data_ptrs[1], pcm_size_mono,
					   CONFIG_AUDIO_BIT_DEPTH_BITS, pcm_data_stereo,
					   &pcm_size_stereo);
			if (ret) {
				return ret;
			}
			break;
		}
		default:
			LOG_ERR("Unsupported channel mode for decoder: %d",
				m_config.decoder.channel_mode);
			return -ENODEV;
		}

		*decoded_size = pcm_size_stereo;
		*decoded_data = pcm_data_stereo;
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return -ENODEV;
	}
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
