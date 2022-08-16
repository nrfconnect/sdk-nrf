/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sw_codec_select.h"

#include <zephyr/kernel.h>
#include <errno.h>

#include "channel_assignment.h"
#include "pcm_stream_channel_modifier.h"
#if (CONFIG_SW_CODEC_LC3)
#include "sw_codec_lc3.h"
#endif /* (CONFIG_SW_CODEC_LC3) */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sw_codec_select);

static struct sw_codec_config m_config;

#if (CONFIG_SW_CODEC_OPUS)
static OpusEncoder *encoder_right_channel;
static OpusEncoder *encoder_left_channel;
static OpusDecoder *decoder;

static int opus_encoder_configure(struct sw_codec_config sw_codec_cfg, OpusEncoder *encoder)
{
	int err;

	err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(sw_codec_cfg.encoder.bitrate));
	if (err < 0) {
		return err;
	}

	err = opus_encoder_ctl(encoder, OPUS_SET_VBR(0));
	if (err < 0) {
		return err;
	}

	err = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(CONFIG_OPUS_COMPLEXITY));
	if (err < 0) {
		return err;
	}

	err = opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
	if (err < 0) {
		return err;
	}

	err = opus_encoder_ctl(encoder, OPUS_SET_LSB_DEPTH(CONFIG_AUDIO_BIT_DEPTH_BITS));
	if (err < 0) {
		return err;
	}

	err = opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_AUTO));
	if (err < 0) {
		return err;
	}

	return err;
}
#endif /*CONFIG_SW_CODEC_OPUS*/

int sw_codec_encode(void *pcm_data, size_t pcm_size, uint8_t **encoded_data, size_t *encoded_size)
{
	/* Temp storage for split stereo PCM signal */
	char pcm_data_mono[AUDIO_CH_NUM][PCM_NUM_BYTES_MONO] = { 0 };
	/* Make sure we have enough space for two frames (stereo) */
	static uint8_t m_encoded_data[ENC_MAX_FRAME_SIZE * AUDIO_CH_NUM];

	size_t pcm_block_size_mono;
	int ret;

	if (!m_config.encoder.enabled) {
		LOG_ERR("Encoder has not been initialized");
		return -ENXIO;
	}

	switch (m_config.sw_codec) {
	case SW_CODEC_OPUS: {
#if (CONFIG_SW_CODEC_OPUS)
		size_t encoded_bytes_written = 0;

		ret = pscm_two_channel_split(pcm_data, pcm_size, CONFIG_AUDIO_BIT_DEPTH_BITS,
					     pcm_data_mono[AUDIO_CH_L], pcm_data_mono[AUDIO_CH_R],
					     &pcm_block_size_mono);
		if (ret) {
			LOG_WRN("Failed to split pcm signal (err: %d)", ret);
			return ret;
		}

		switch (m_config.encoder.channel_mode) {
		case SW_CODEC_MONO: {
			encoded_bytes_written =
				opus_encode(encoder_left_channel,
					    (int16_t *)pcm_data_mono[m_config.encoder.audio_ch],
					    OPUS_PCM_FRAME_SIZE_SAMPLES_MONO, m_encoded_data,
					    sizeof(m_encoded_data));
			if (encoded_bytes_written <= 0) {
				LOG_ERR("Failed to encode (err: %d)", encoded_bytes_written);
				return encoded_bytes_written;
			}
			break;
		}
		case SW_CODEC_STEREO: {
			ret = opus_encode(encoder_left_channel,
					  (int16_t *)pcm_data_mono[AUDIO_CH_L],
					  OPUS_PCM_FRAME_SIZE_SAMPLES_MONO, m_encoded_data,
					  sizeof(m_encoded_data));
			if (ret <= 0) {
				LOG_ERR("Failed to encode (Left channel) (err: %d)", ret);
				return ret;
			}

			encoded_bytes_written += ret;
			ret = opus_encode(encoder_right_channel,
					  (int16_t *)pcm_data_mono[AUDIO_CH_R],
					  OPUS_PCM_FRAME_SIZE_SAMPLES_MONO,
					  m_encoded_data + encoded_bytes_written,
					  sizeof(m_encoded_data) - encoded_bytes_written);
			if (ret <= 0) {
				LOG_ERR("Failed to encode (Right channel) (err: %d)", ret);
				return ret;
			}

			encoded_bytes_written += encoded_bytes_written;
			break;
		}
		default:
			LOG_ERR("Unsupported channel mode: %d", m_config.encoder.channel_mode);
			return -ENODEV;
		}

		*encoded_data = m_encoded_data;
		*encoded_size = encoded_bytes_written;

#endif /* (CONFIG_SW_CODEC_OPUS) */
		break;
	}

	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		uint16_t encoded_bytes_written;

		/* Since LC3 is a single channel codec, we must split the
		 * stereo PCM stream
		 */
		ret = pscm_two_channel_split(pcm_data, pcm_size, CONFIG_AUDIO_BIT_DEPTH_BITS,
					     pcm_data_mono[AUDIO_CH_L], pcm_data_mono[AUDIO_CH_R],
					     &pcm_block_size_mono);
		if (ret) {
			return ret;
		}

		switch (m_config.encoder.channel_mode) {
		case SW_CODEC_MONO: {
			ret = sw_codec_lc3_enc_run(pcm_data_mono[m_config.encoder.audio_ch],
						   pcm_block_size_mono, LC3_USE_BITRATE_FROM_INIT,
						   0, sizeof(m_encoded_data), m_encoded_data,
						   &encoded_bytes_written);
			if (ret) {
				return ret;
			}
			break;
		}
		case SW_CODEC_STEREO: {
			ret = sw_codec_lc3_enc_run(pcm_data_mono[AUDIO_CH_L], pcm_block_size_mono,
						   LC3_USE_BITRATE_FROM_INIT, AUDIO_CH_L,
						   sizeof(m_encoded_data), m_encoded_data,
						   &encoded_bytes_written);
			if (ret) {
				return ret;
			}

			ret = sw_codec_lc3_enc_run(pcm_data_mono[AUDIO_CH_R], pcm_block_size_mono,
						   LC3_USE_BITRATE_FROM_INIT, AUDIO_CH_R,
						   sizeof(m_encoded_data) - encoded_bytes_written,
						   m_encoded_data + encoded_bytes_written,
						   &encoded_bytes_written);
			if (ret) {
				return ret;
			}
			encoded_bytes_written += encoded_bytes_written;
			break;
		}
		default:
			LOG_ERR("Unsupported channel mode: %d", m_config.encoder.channel_mode);
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
	int16_t pcm_data_mono[PCM_NUM_BYTES_MONO] = { 0 };
	static char pcm_data_stereo[PCM_NUM_BYTES_STEREO];

	size_t pcm_size_stereo = 0;
	size_t pcm_size_session = 0;

	switch (m_config.sw_codec) {
	case SW_CODEC_OPUS: {
#if (CONFIG_SW_CODEC_OPUS)
		switch (m_config.decoder.channel_mode) {
		case SW_CODEC_MONO: {
			pcm_size_session =
				opus_decode(decoder, encoded_data, encoded_size, pcm_data_mono,
					    OPUS_PCM_FRAME_SIZE_SAMPLES_MONO, 0);
			if (pcm_size_session < 0) {
				LOG_ERR("Failed to decode (err: %d)", pcm_size_session);
				return pcm_size_session;
			}
			/* 16 bits per samples */
			pcm_size_session *= CONFIG_AUDIO_BIT_DEPTH_OCTETS;
			ret = pscm_zero_pad(pcm_data_mono, pcm_size_session,
					    m_config.decoder.audio_ch, CONFIG_AUDIO_BIT_DEPTH_BITS,
					    pcm_data_stereo, &pcm_size_stereo);
			if (ret) {
				return ret;
			}
			break;
		}
		case SW_CODEC_STEREO: {
			LOG_WRN("Stereo decoding is not supported for OPUS yet");
			break;
		}
		default:
			LOG_ERR("Unsupported channel mode: %d", m_config.decoder.channel_mode);
			return -ENODEV;
		}

		*decoded_size = pcm_size_stereo;
		*decoded_data = pcm_data_stereo;
#endif /* (CONFIG_SW_CODEC_OPUS) */
		break;
	}
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		/* Typically used for right channel if stereo signal */
		char pcm_data_mono_right[PCM_NUM_BYTES_MONO] = { 0 };

		switch (m_config.decoder.channel_mode) {
		case SW_CODEC_MONO: {
			if (bad_frame && IS_ENABLED(CONFIG_SW_CODEC_OVERRIDE_PLC)) {
				memset(pcm_data_mono, 0, PCM_NUM_BYTES_MONO);
				pcm_size_session = PCM_NUM_BYTES_MONO;
			} else {
				ret = sw_codec_lc3_dec_run(encoded_data, encoded_size,
							   LC3_PCM_NUM_BYTES_MONO, 0, pcm_data_mono,
							   (uint16_t *)&pcm_size_session,
							   bad_frame);
				if (ret) {
					return ret;
				}
			}

			/* For now, i2s is only stereo, so in order to send
			 * just one channel, we need to insert 0 for the
			 * other channel
			 */
			ret = pscm_zero_pad(pcm_data_mono, pcm_size_session,
					    m_config.decoder.audio_ch, CONFIG_AUDIO_BIT_DEPTH_BITS,
					    pcm_data_stereo, &pcm_size_stereo);
			if (ret) {
				return ret;
			}
			break;
		}
		case SW_CODEC_STEREO: {
			if (bad_frame && IS_ENABLED(CONFIG_SW_CODEC_OVERRIDE_PLC)) {
				memset(pcm_data_mono, 0, PCM_NUM_BYTES_MONO);
				memset(pcm_data_mono_right, 0, PCM_NUM_BYTES_MONO);
				pcm_size_session = PCM_NUM_BYTES_MONO;
			} else {
				/* Decode left channel */
				ret = sw_codec_lc3_dec_run(encoded_data, encoded_size / 2,
							   LC3_PCM_NUM_BYTES_MONO, AUDIO_CH_L,
							   pcm_data_mono,
							   (uint16_t *)&pcm_size_session,
							   bad_frame);
				if (ret) {
					return ret;
				}
				/* Decode right channel */
				ret = sw_codec_lc3_dec_run((encoded_data + (encoded_size / 2)),
							   encoded_size / 2, LC3_PCM_NUM_BYTES_MONO,
							   AUDIO_CH_R, pcm_data_mono_right,
							   (uint16_t *)&pcm_size_session,
							   bad_frame);
				if (ret) {
					return ret;
				}
			}
			ret = pscm_combine(pcm_data_mono, pcm_data_mono_right, pcm_size_session,
					   CONFIG_AUDIO_BIT_DEPTH_BITS, pcm_data_stereo,
					   &pcm_size_stereo);
			if (ret) {
				return ret;
			}
			break;
		}
		default:
			LOG_ERR("Unsupported channel mode: %d", m_config.encoder.channel_mode);
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
	case SW_CODEC_OPUS:
#if (CONFIG_SW_CODEC_OPUS)
		if (sw_codec_cfg.encoder.enabled) {
			if (!m_config.encoder.enabled) {
				LOG_ERR("Trying to uninit encoder, it has not been initialized");
				return -EALREADY;
			}

			opus_encoder_destroy(encoder_left_channel);
			opus_encoder_destroy(encoder_right_channel);
			m_config.encoder.enabled = false;
		}

		if (sw_codec_cfg.decoder.enabled) {
			if (!m_config.decoder.enabled) {
				LOG_WRN("Trying to uninit decoder, it has not been initialized");
				return -EALREADY;
			}

			opus_decoder_destroy(decoder);
			m_config.decoder.enabled = false;
		}
#endif /* (CONFIG_SW_CODEC_OPUS) */
		ret = 0;
		break;

	case SW_CODEC_LC3:
#if (CONFIG_SW_CODEC_LC3)
		if (sw_codec_cfg.encoder.enabled) {
			if (!m_config.encoder.enabled) {
				LOG_ERR("Trying to uninit encoder, it has not been initialized");
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
				LOG_WRN("Trying to uninit decoder, it has not been initialized");
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
	return 0;
}

int sw_codec_init(struct sw_codec_config sw_codec_cfg)
{
	switch (sw_codec_cfg.sw_codec) {
	case SW_CODEC_OPUS: {
#if (CONFIG_SW_CODEC_OPUS)
		int err;

		if (sw_codec_cfg.encoder.enabled) {
			if (m_config.encoder.enabled) {
				LOG_WRN("The OPUS encoder is already initialized");
				return -EALREADY;
			}

			encoder_left_channel = opus_encoder_create(
				CONFIG_OPUS_SAMPLE_RATE, CONFIG_OPUS_CHANNELS, APPLICATION, &err);
			if (err < 0) {
				LOG_ERR("Failed to create the OPUS Encoder (err: %d)", err);
				return err;
			}

			err = opus_encoder_configure(sw_codec_cfg, encoder_left_channel);
			if (err) {
				LOG_ERR("Failed to configure OPUS encoder (err: %d)", err);
				return err;
			}

			encoder_right_channel = opus_encoder_create(
				CONFIG_OPUS_SAMPLE_RATE, CONFIG_OPUS_CHANNELS, APPLICATION, &err);
			if (err < 0) {
				LOG_ERR("Failed to create the OPUS Encoder (err: %d)", err);
				return err;
			}

			err = opus_encoder_configure(sw_codec_cfg, encoder_right_channel);
			if (err) {
				LOG_ERR("Failed to configure OPUS encoder (err: %d)", err);
				return err;
			}
		}

		if (sw_codec_cfg.decoder.enabled) {
			if (m_config.decoder.enabled) {
				LOG_WRN("The OPUS decoder is already initialized");
				return -EALREADY;
			}

			decoder = opus_decoder_create(CONFIG_OPUS_SAMPLE_RATE, CONFIG_OPUS_CHANNELS,
						      &err);
			if (err < 0) {
				LOG_ERR("Failed to create OPUS Decoder");
				return err;
			}
		}
		break;
#endif /*CONFIG_SW_CODEC_OPUS */
		LOG_ERR("OPUS is not compiled in, please open menuconfig and select OPUS");
		return -ENODEV;
	}
	case SW_CODEC_LC3: {
#if (CONFIG_SW_CODEC_LC3)
		int ret;

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

			ret = sw_codec_lc3_enc_init(
				CONFIG_AUDIO_SAMPLE_RATE_HZ, CONFIG_AUDIO_BIT_DEPTH_BITS,
				CONFIG_AUDIO_FRAME_DURATION_US, sw_codec_cfg.encoder.bitrate,
				sw_codec_cfg.encoder.channel_mode, &pcm_bytes_req_enc);
			if (ret) {
				return ret;
			}
		}

		if (sw_codec_cfg.decoder.enabled) {
			if (m_config.decoder.enabled) {
				LOG_WRN("The LC3 decoder is already initialized");
				return -EALREADY;
			}
			ret = sw_codec_lc3_dec_init(CONFIG_AUDIO_SAMPLE_RATE_HZ,
						    CONFIG_AUDIO_BIT_DEPTH_BITS,
						    CONFIG_AUDIO_FRAME_DURATION_US,
						    sw_codec_cfg.decoder.channel_mode);
			if (ret) {
				return ret;
			}
		}
		break;
#endif /* (CONFIG_SW_CODEC_LC3) */
		LOG_ERR("LC3 is not compiled in, please open menuconfig and select LC3");
		return -ENODEV;
	}
	default:
		LOG_ERR("Unsupported codec: %d", sw_codec_cfg.sw_codec);
		return false;
	}

	m_config = sw_codec_cfg;
	return 0;
}
