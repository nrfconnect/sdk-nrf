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

#include "macros_common.h"

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
		uint8_t inter_buf[PCM_NUM_BYTES_MONO];
		uint8_t src_buf[PCM_NUM_BYTES_MONO];
		uint8_t chan_in_num, chan_out_num;
		uint8_t chan_out = 0;
		uint8_t *inter_out;
		uint8_t *enc_in = audio_frame_in->data;
		uint8_t *enc_out = audio_frame_out->data;
		size_t enc_in_size = 0;
		uint16_t bytes_written;
		uint32_t loc_in = 0;
		uint32_t loc_out = 0;

		LOG_DBG("LC3 encoder module");

		if ((meta_in->data_coding != PCM) || (meta_out->data_coding != LC3)) {
			LOG_ERR("LC3 encoder module has incorrect input or output data type: in = "
				"%d  out = %d",
				meta_in->data_coding, meta_out->data_coding);
			return -EINVAL;
		}

		chan_in_num = audio_metadata_num_loc_get(meta_in);
		if (audio_frame_in->len < (meta_in->bytes_per_location * chan_in_num)) {
			LOG_ERR("Encoder input buffer too small: %d (>=%d)", audio_frame_in->len,
				meta_in->bytes_per_location * chan_in_num);
			return -EINVAL;
		}

		chan_out_num = audio_metadata_num_loc_get(meta_out);
		if (audio_frame_out->size < (meta_out->bytes_per_location * chan_out_num)) {
			LOG_ERR("Encoder output buffer too small: %d (>=%d)", audio_frame_out->size,
				meta_out->bytes_per_location * chan_out_num);
			return -EINVAL;
		}

		if (meta_in->locations == BT_AUDIO_LOCATION_MONO_AUDIO) {
			loc_in = 1;
		} else {
			loc_in = meta_in->locations;
		}

		if (meta_out->locations == BT_AUDIO_LOCATION_MONO_AUDIO) {
			/* Set output to be the lowest set location in meta_in->locations */
			loc_out = 1;
			while (loc_out && !(meta_in->locations & loc_out)) {
				loc_out <<= 1;
			}
		} else {
			loc_out = meta_out->locations;
		}

		if (loc_out == 0 || loc_in == 0) {
			LOG_ERR("No common output location with input");
			LOG_ERR("Input locations:  0x%08x", meta_in->locations);
			LOG_ERR("Output locations: 0x%08x", meta_out->locations);
			return -EINVAL;
		}

		/* Encode only the common channel(s) between the input and output locations. */
		while (loc_out && loc_in) {
			if (loc_out & loc_in & 0x01) {
				if (meta_in->interleaved) {
					ret = pscm_deinterleave(audio_frame_in->data,
								audio_frame_in->len, chan_in_num,
								chan_out,
								meta_in->carried_bits_per_sample,
								inter_buf, sizeof(inter_buf));
					ERR_CHK_MSG(ret, "Encode: Failed de-interleaving");

					inter_out = inter_buf;
				} else {
					inter_out = (uint8_t *)audio_frame_in->data +
						    (meta_in->bytes_per_location * chan_out);
				}

				ret = sw_codec_sample_rate_convert(
					&encoder_converters[chan_out], meta_in->sample_rate_hz,
					meta_out->sample_rate_hz, inter_out,
					meta_in->bytes_per_location, src_buf, (char **)&enc_in,
					&enc_in_size);
				ERR_CHK_MSG(ret, "Encode: Sample rate conversion failed");

				ret = sw_codec_lc3_enc_run(
					enc_in, enc_in_size, meta_out->bitrate_bps, chan_out,
					meta_in->bytes_per_location, enc_out, &bytes_written);
				ERR_CHK_MSG(ret, "Encode failed");

				enc_out += bytes_written;

				LOG_DBG("Completed LC3 encode of ch: %d", chan_out);
			}

			chan_out += loc_out & 0x01;

			loc_in >>= 1;
			loc_out >>= 1;
		}

		if (IS_ENABLED(CONFIG_MONO_TO_ALL_RECEIVERS) && (m_config.encoder.num_ch > 1)) {
			/* Duplicate the mono encoded data to all output locations */
			size_t single_chan_size = bytes_written;

			for (uint8_t ch = 1; ch < m_config.encoder.num_ch; ch++) {
				memcpy(audio_frame_out->data + (ch * single_chan_size),
				       audio_frame_out->data, single_chan_size);
			}
		}

		meta_out->bytes_per_location = bytes_written;
		meta_out->locations &= meta_in->locations;
		net_buf_add(audio_frame_out,
			    meta_out->bytes_per_location * audio_metadata_num_loc_get(meta_out));

		return 0;
#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", meta_out->data_coding);
		return -ENODEV;
	}

	return 0;
}

int sw_codec_decode(struct net_buf const *const audio_frame_in,
		    struct net_buf *const audio_frame_out)
{
	if (audio_frame_in == NULL || audio_frame_out == NULL) {
		return -EINVAL;
	}

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
		uint8_t dec_out_buf[PCM_NUM_BYTES_MONO];
		uint8_t src_buf[PCM_NUM_BYTES_MONO];
		uint8_t *data_in;
		uint8_t *dec_out = dec_out_buf;
		uint8_t *src_out = src_buf;
		uint8_t chan_in, chan_out;
		uint8_t chans_out_num;
		uint8_t *inter_in = dec_out_buf;
		uint16_t bytes_written;
		uint32_t loc_in, loc_out;
		uint32_t bad_data_mask;
		size_t inter_in_size = 0;

		if (meta_in->data_coding != LC3 || meta_out->data_coding != PCM) {
			LOG_ERR("LC3 decoder module has incorrect input or output data type: in = "
				"%d  out = %d",
				meta_in->data_coding, meta_out->data_coding);
			return -EINVAL;
		}

		if (audio_frame_in->len <
		    meta_in->bytes_per_location * audio_metadata_num_loc_get(meta_in)) {
			LOG_ERR("Decoder input frame too small: %d (< %d)", audio_frame_in->len,
				(meta_in->bytes_per_location *
				 audio_metadata_num_loc_get(meta_in)));
			return -EINVAL;
		}

		chans_out_num = audio_metadata_num_loc_get(meta_out);
		if (audio_frame_out->size < meta_out->bytes_per_location * chans_out_num) {
			LOG_ERR("Decoder output buffer too small: %d (<%d for %d channel(s))",
				audio_frame_out->size,
				(meta_out->bytes_per_location * chans_out_num), chans_out_num);
			return -EINVAL;
		}

		if (meta_out->locations == BT_AUDIO_LOCATION_MONO_AUDIO &&
		    meta_in->locations == BT_AUDIO_LOCATION_MONO_AUDIO) {
			loc_in = 1;
			loc_out = 1;
		} else if (meta_out->locations & meta_in->locations) {
			loc_in = meta_in->locations;
			loc_out = meta_out->locations;
		} else {
			LOG_ERR("No common output location with input");
			return -EINVAL;
		}

		if (!meta_out->interleaved) {
			if (IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER) &&
			    meta_in->sample_rate_hz != meta_out->sample_rate_hz) {
				src_out = (uint8_t *)audio_frame_out->data;
			} else {
				dec_out = (uint8_t *)audio_frame_out->data;
			}
		}

		/* Clear all output channels to ensure any unused are zero */
		memset(audio_frame_out->data, 0, audio_frame_out->size);

		chan_in = 0;
		chan_out = 0;
		bad_data_mask = 0x01;

		/* Decode only the channel(s) common between the input and output locations.
		 * These will be put in the first channel or channels and the location
		 * will indicate which channel(s) they are. Prior to playout (I2S or TDM)
		 * all other channels can be zeroed.
		 */
		while (loc_in && loc_out) {
			if (loc_out & loc_in & 0x01) {
				data_in = (uint8_t *)audio_frame_in->data +
					  (meta_in->bytes_per_location * chan_in);

				ret = sw_codec_lc3_dec_run(data_in, meta_in->bytes_per_location,
							   audio_frame_out->size, chan_in, dec_out,
							   &bytes_written,
							   (meta_in->bad_data & bad_data_mask));
				ERR_CHK_MSG(ret, "Decode failed");

				ret = sw_codec_sample_rate_convert(
					&decoder_converters[chan_in], meta_in->sample_rate_hz,
					meta_out->sample_rate_hz, dec_out, bytes_written, src_out,
					(char **)&inter_in, &inter_in_size);
				ERR_CHK_MSG(ret, "Decode: Sample rate converter failed");

				if (meta_out->interleaved) {
					ret = pscm_interleave(inter_in, inter_in_size, chan_out,
							      meta_out->carried_bits_per_sample,
							      audio_frame_out->data,
							      audio_frame_out->size, chans_out_num);
					ERR_CHK_MSG(ret, "Decode: Interleave failed");
				} else {
					if (IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER) &&
					    meta_in->sample_rate_hz != meta_out->sample_rate_hz) {
						src_out += inter_in_size;
					} else {
						dec_out += inter_in_size;
					}
				}
			}

			bad_data_mask <<= 1;

			chan_in += loc_in & 0x01;
			chan_out += loc_out & 0x01;

			loc_in >>= 1;
			loc_out >>= 1;
		}

		meta_out->bytes_per_location = inter_in_size;
		net_buf_add(audio_frame_out,
			    meta_out->bytes_per_location * audio_metadata_num_loc_get(meta_out));

#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", meta_in->data_coding);
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

	if (sw_codec_cfg.encoder.num_ch > CONFIG_AUDIO_ENCODE_CHANNELS_MAX) {
		LOG_ERR("Number of encoder channels %d exceeds max %d", sw_codec_cfg.encoder.num_ch,
			CONFIG_AUDIO_ENCODE_CHANNELS_MAX);
		return -EINVAL;
	}

	if (sw_codec_cfg.encoder.enabled && IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER)) {
		for (int i = 0; i < sw_codec_cfg.encoder.num_ch; i++) {
			ret = sample_rate_converter_open(&encoder_converters[i]);
			if (ret) {
				LOG_ERR("Failed to initialize the sample rate converter for "
					"encoding channel %d: %d",
					i, ret);
				return ret;
			}
		}
	}

	if (sw_codec_cfg.decoder.num_ch > CONFIG_AUDIO_DECODE_CHANNELS_MAX) {
		LOG_ERR("Number of decoder channels %d exceeds max %d", sw_codec_cfg.decoder.num_ch,
			CONFIG_AUDIO_DECODE_CHANNELS_MAX);
		return -EINVAL;
	}

	if (sw_codec_cfg.decoder.enabled && IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER)) {
		for (int i = 0; i < sw_codec_cfg.decoder.num_ch; i++) {
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
