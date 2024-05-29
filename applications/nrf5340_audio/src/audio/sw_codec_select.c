/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sw_codec_select.h"

#include <zephyr/kernel.h>
#include <errno.h>
#include <pcm_stream_channel_modifier.h>
#include <audio_module.h>
#include <sample_rate_converter.h>

#if (CONFIG_SW_CODEC_LC3)
#include <zephyr/bluetooth/audio/audio.h>

#include "lc3_decoder.h"
#include "sw_codec_lc3.h"
#endif /* (CONFIG_SW_CODEC_LC3) */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sw_codec_select, CONFIG_SW_CODEC_SELECT_LOG_LEVEL);

static struct sw_codec_config m_config;

static struct sample_rate_converter_ctx encoder_converters[AUDIO_CH_NUM];
static struct sample_rate_converter_ctx decoder_converters[AUDIO_CH_NUM];

#define LC3_DECODER_MSG_QUEUE_SIZE (2)
#define LC3_DATA_OBJECTS_NUM	   (2)

/* @brief Index into the module handles array for a given module. */
enum stream_module_id {
	LC3_MODULE_ID_DECODER = 0,
	LC3_MODULE_ID_ENCODER,
	LC3_MODULE_ID_NUM
};

/* Streams module handles */
struct audio_module_handle handle[LC3_MODULE_ID_NUM];

K_THREAD_STACK_DEFINE(lc3_dec_thread_stack, CONFIG_LC3_DECODER_STACK_SIZE);
DATA_FIFO_DEFINE(lc3_dec_fifo_tx, LC3_DECODER_MSG_QUEUE_SIZE, sizeof(struct audio_module_message));
DATA_FIFO_DEFINE(lc3_dec_fifo_rx, LC3_DECODER_MSG_QUEUE_SIZE, sizeof(struct audio_module_message));

static char audio_data_memory[PCM_NUM_BYTES_MONO * LC3_DATA_OBJECTS_NUM];
struct k_mem_slab audio_data_slab;

/* Decoder modules private context */
struct lc3_decoder_context decoder_ctx;

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

int sw_codec_encode(void *pcm_data, size_t pcm_size, uint8_t **encoded_data, size_t *encoded_size)
{
	int ret;

	/* Temp storage for split stereo PCM signal */
	char pcm_data_mono_system_sample_rate[AUDIO_CH_NUM][PCM_NUM_BYTES_MONO] = {0};
	/* Make sure we have enough space for two frames (stereo) */
	static uint8_t m_encoded_data[ENC_MAX_FRAME_SIZE * AUDIO_CH_NUM];

	char pcm_data_mono_converted_buf[AUDIO_CH_NUM][PCM_NUM_BYTES_MONO] = {0};

	size_t pcm_block_size_mono_system_sample_rate;
	size_t pcm_block_size_mono;

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
		ret = pscm_two_channel_split(pcm_data, pcm_size, CONFIG_AUDIO_BIT_DEPTH_BITS,
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
						   0, sizeof(m_encoded_data), m_encoded_data,
						   &encoded_bytes_written);
			if (ret) {
				return ret;
			}
			break;
		}
		case SW_CODEC_STEREO: {
			ret = sw_codec_lc3_enc_run(pcm_data_mono_ptrs[AUDIO_CH_L],
						   pcm_block_size_mono, LC3_USE_BITRATE_FROM_INIT,
						   AUDIO_CH_L, sizeof(m_encoded_data),
						   m_encoded_data, &encoded_bytes_written);
			if (ret) {
				return ret;
			}

			ret = sw_codec_lc3_enc_run(
				pcm_data_mono_ptrs[AUDIO_CH_R], pcm_block_size_mono,
				LC3_USE_BITRATE_FROM_INIT, AUDIO_CH_R,
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

		*encoded_data = m_encoded_data;
		*encoded_size = encoded_bytes_written;

#endif /* (CONFIG_SW_CODEC_LC3) */
		break;
	}
	default:
		LOG_ERR("Unsupported codec: %d", m_config.sw_codec);
		return -ENODEV;
	}

	return 0;
}

int sw_codec_decode(uint8_t const *const encoded_data, size_t encoded_size, bool bad_frame,
		    void **decoded_data, size_t *decoded_size)
{
	if (!m_config.decoder.enabled) {
		LOG_ERR("Decoder has not been initialized");
		return -ENXIO;
	}

	int ret;

	static char pcm_data_stereo[PCM_NUM_BYTES_STEREO];

	char decoded_data_stereo[PCM_NUM_BYTES_MONO * m_config.decoder.num_ch];
	char decoded_data_stereo_system_sample_rate[PCM_NUM_BYTES_MONO * m_config.decoder.num_ch];

	char *dec_data;
	char *dec_data_resampled;

	size_t pcm_size_stereo = 0;
	size_t pcm_size_mono = 0;
	size_t decoded_data_size = 0;

	switch (m_config.sw_codec) {
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		if (bad_frame && IS_ENABLED(CONFIG_SW_CODEC_OVERRIDE_PLC)) {
			/* Could just clear the stereo buffer and update the size to not have to do
			 * resampling on a zero buffer.
			 */

			memset(dec_data, 0, PCM_NUM_BYTES_MONO * m_config.decoder.num_ch);
			decoded_data_size = PCM_NUM_BYTES_MONO * m_config.decoder.num_ch;
		} else {
			struct audio_data audio_data_tx;
			struct audio_data audio_data_rx;

			/* The setting of the meta data will happen when the audio data is passed
			 * from the BT to the audio framework.
			 */
			audio_data_tx.data = (void *)encoded_data;
			audio_data_tx.data_size = encoded_size;
			audio_data_tx.meta.data_coding = LC3;
			audio_data_tx.meta.data_len_us = CONFIG_AUDIO_FRAME_DURATION_US;
			audio_data_tx.meta.sample_rate_hz = m_config.decoder.sample_rate_hz;
			audio_data_tx.meta.bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			audio_data_tx.meta.carried_bits_pr_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			if (m_config.decoder.channel_mode == SW_CODEC_MONO) {
				audio_data_tx.meta.locations = BIT(m_config.decoder.audio_ch);
			} else {
				audio_data_tx.meta.locations = BT_AUDIO_LOCATION_FRONT_LEFT +
							       BT_AUDIO_LOCATION_FRONT_RIGHT;
			}
			audio_data_tx.meta.bad_data = bad_frame;

			audio_data_rx.data = (void *)decoded_data_stereo;
			audio_data_rx.data_size = PCM_NUM_BYTES_STEREO;
			audio_data_rx.meta.data_coding = PCM;
			audio_data_rx.meta.data_len_us = CONFIG_AUDIO_FRAME_DURATION_US;
			audio_data_rx.meta.sample_rate_hz = m_config.decoder.sample_rate_hz;
			audio_data_rx.meta.bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			audio_data_rx.meta.carried_bits_pr_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			audio_data_rx.meta.locations = audio_data_tx.meta.locations;

			ret = audio_module_data_tx_rx(&handle[LC3_MODULE_ID_DECODER],
						      &handle[LC3_MODULE_ID_DECODER],
						      &audio_data_tx, &audio_data_rx, K_FOREVER);
			if (ret) {
				return ret;
			}
		}

		dec_data = decoded_data_stereo;
		dec_data_resampled = decoded_data_stereo_system_sample_rate;

		for (int i = 0; i < m_config.decoder.num_ch; ++i) {
			ret = sw_codec_sample_rate_convert(
				&decoder_converters[i], m_config.decoder.sample_rate_hz,
				CONFIG_AUDIO_SAMPLE_RATE_HZ, dec_data, PCM_NUM_BYTES_MONO,
				dec_data_resampled, &dec_data, &pcm_size_mono);
			if (ret) {
				LOG_ERR("Sample rate conversion failed for channel "
					"%d : %d",
					i, ret);
				return ret;
			}

			dec_data += PCM_NUM_BYTES_MONO;
			dec_data_resampled += PCM_NUM_BYTES_MONO;
		}

		/* For now, i2s is only stereo, so in order to send
		 * just one channel, we need to insert 0 for the
		 * other channel.
		 */
		if (m_config.decoder.channel_mode == SW_CODEC_MONO) {
			ret = pscm_zero_pad(decoded_data_stereo, pcm_size_mono,
					    m_config.decoder.audio_ch, CONFIG_AUDIO_BIT_DEPTH_BITS,
					    pcm_data_stereo, &pcm_size_stereo);
			if (ret) {
				return ret;
			}
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

			ret = audio_module_stop(&handle[LC3_MODULE_ID_DECODER]);
			if (ret) {
				LOG_WRN("Failed to stop decoder module: %d", ret);
				return ret;
			}

			ret = audio_module_close(&handle[LC3_MODULE_ID_DECODER]);
			if (ret) {
				LOG_WRN("Failed to close decoder module: %d", ret);
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
			struct audio_module_parameters audio_decoder_param;
			struct lc3_decoder_configuration decoder_config;

			if (m_config.decoder.enabled) {
				LOG_WRN("The LC3 decoder is already initialized");
				return -EALREADY;
			}

			memset(&handle[LC3_MODULE_ID_DECODER], 0,
			       sizeof(struct audio_module_handle));

			decoder_config.sample_rate_hz = sw_codec_cfg.decoder.sample_rate_hz;
			decoder_config.bits_per_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			decoder_config.carried_bits_pr_sample = CONFIG_AUDIO_BIT_DEPTH_BITS;
			decoder_config.data_len_us = CONFIG_AUDIO_FRAME_DURATION_US;
			/* Once the resampler is either moved into the decoder/encoder or I2S this
			 * should be true.
			 */
			decoder_config.interleaved = false;
			if (sw_codec_cfg.decoder.channel_mode == SW_CODEC_MONO) {
				decoder_config.locations = BIT(sw_codec_cfg.decoder.audio_ch);
			} else {
				decoder_config.locations = BT_AUDIO_LOCATION_FRONT_LEFT +
							   BT_AUDIO_LOCATION_FRONT_RIGHT;
			}
			decoder_config.bitrate_bps_max = CONFIG_LC3_BITRATE;

			data_fifo_init(&lc3_dec_fifo_rx);
			data_fifo_init(&lc3_dec_fifo_tx);

			ret = k_mem_slab_init(&audio_data_slab, &audio_data_memory[0],
					      PCM_NUM_BYTES_MONO * sw_codec_cfg.decoder.num_ch,
					      LC3_DATA_OBJECTS_NUM);
			if (ret) {
				LOG_ERR("Failed to allocate the data slab: %d", ret);
				return ret;
			}

			AUDIO_MODULE_PARAMETERS(audio_decoder_param, lc3_decoder_description,
						lc3_dec_thread_stack, CONFIG_LC3_DECODER_STACK_SIZE,
						CONFIG_LC3_DECODER_THREAD_PRIO, lc3_dec_fifo_rx,
						lc3_dec_fifo_tx, audio_data_slab,
						PCM_NUM_BYTES_MONO * sw_codec_cfg.decoder.num_ch);

			ret = audio_module_open(
				&audio_decoder_param,
				(struct audio_module_configuration *)&decoder_config, "LC3 Decoder",
				(struct audio_module_context *)&decoder_ctx,
				&handle[LC3_MODULE_ID_DECODER]);

			ret = audio_module_connect(&handle[LC3_MODULE_ID_DECODER], NULL, true);
			if (ret) {
				LOG_ERR("Decoder connect faild, %d", ret);
				return ret;
			}

			ret = audio_module_start(&handle[LC3_MODULE_ID_DECODER]);
			if (ret) {
				LOG_ERR("Decoder start faild, %d", ret);
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
