/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Fixed-profile G.722 codec helpers used by the software codec selector.
 */

#ifndef SW_CODEC_G722_H_
#define SW_CODEC_G722_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct audio_metadata;
struct sw_codec_config;
struct sample_rate_converter_ctx;

/**
 * @brief Check frame metadata against the fixed G.722 profile and channel limits.
 *
 * Reports independent metadata validation failures before returning. Derived frame-size
 * and channel-count checks run only when their format and location prerequisites are valid.
 *
 * @param[in]	pcm	Non-NULL application-rate PCM metadata.
 * @param[in]	encoded	Non-NULL codec-rate metadata for 80-byte mono channel frames.
 * @param[in]	encoder	True for encoding; false for decoding. Initial decoder output
			block size is not required to describe a complete 10 ms frame.
 * @param[in]	current	Non-NULL selector state supplying initialized channel limits.
 *
 * @retval	0	Metadata is supported.
 * @retval	-EINVAL	Unsupported format, frame size, rate or channel locations.
 */
int sw_codec_g722_metadata_check(const struct audio_metadata *pcm,
				 const struct audio_metadata *encoded, bool encoder,
				 const struct sw_codec_config *current);

/**
 * @brief Validate a G.722 input frame, allowing absent payload only for lost channels.
 *
 * @param[in]	payload_size	Number of encoded bytes present in the input frame.
 * @param[in]	encoded		Metadata describing the encoded G.722 frame.
 * @param[in]	pcm		Metadata describing the decoded PCM frame.
 * @param[in]	current		Initialized selector configuration.
 *
 * @retval	0	Frame metadata and payload length are valid.
 * @retval	-EINVAL	Unsupported metadata, oversized payload, or truncated valid channel.
 */
int sw_codec_g722_decode_validate(size_t payload_size, const struct audio_metadata *encoded,
				  const struct audio_metadata *pcm,
				  const struct sw_codec_config *current);

/**
 * @brief Destroy all encoder contexts, including a partially allocated set.
 *
 * Cleared slots make repeated cleanup safe. Decoder contexts are not affected.
 */
void sw_codec_g722_enc_uninit_all(void);

/**
 * @brief Destroy all decoder contexts, including a partially allocated set.
 *
 * Cleared slots make repeated cleanup safe. Encoder contexts are not affected.
 */
void sw_codec_g722_dec_uninit_all(void);

/**
 * @brief Encode one mono 10 ms frame without allocating memory.
 *
 * @param[in]	pcm_data		Non-NULL, int16-aligned PCM samples.
 * @param[in]	pcm_data_size		Input size in bytes; exactly 320 (160 samples).
 * @param[in]	bitrate			Encoded bitrate; must be 64000 bit/s.
 * @param[in]	audio_ch		Initialized encoder context index, not a location bit.
 * @param[in]	g722_data_buf_size	Output capacity in bytes.
 * @param[out]	g722_data		Non-NULL output storage for at least 80 bytes.
 * @param[out]	g722_data_wr_size	Non-NULL destination for bytes written on success.
 *
 * @retval	0	Exactly 80 encoded bytes written.
 * @retval	-EINVAL	Invalid context, profile, input size or alignment.
 * @retval	-ENOMEM	Insufficient output capacity.
 * @retval	-EIO	Unexpected library output length.
 */
int sw_codec_g722_enc_run(void const *const pcm_data, uint32_t pcm_data_size, uint32_t bitrate,
			  uint8_t audio_ch, uint16_t g722_data_buf_size, uint8_t *const g722_data,
			  uint16_t *const g722_data_wr_size);

/**
 * @brief Decode one mono 10 ms frame, or emit silence for a lost channel.
 *
 * @param[in]	g722_data		Encoded input; may be NULL only when @p bad_frame is true.
 * @param[in]	g722_data_size		Input bytes; must be 80 unless @p bad_frame is true.
 * @param[in]	pcm_data_buf_size	Output capacity in bytes.
 * @param[in]	audio_ch		Initialized decoder context index, not a location bit.
 * @param[out]	pcm_data		Non-NULL, int16-aligned storage for 320 PCM bytes.
 * @param[out]	pcm_data_wr_size	Non-NULL destination for bytes written on success.
 * @param[in]	bad_frame		Ignore payload and emit silence without advancing predictor
 *					state.
 *
 * @retval	0	Exactly 320 PCM bytes written.
 * @retval	-EINVAL	Invalid context or valid-frame payload.
 * @retval	-ENOMEM	Insufficient output capacity or misaligned PCM storage.
 * @retval	-EIO	Unexpected library sample count.
 */
int sw_codec_g722_dec_run(uint8_t const *const g722_data, uint16_t g722_data_size,
			  uint16_t pcm_data_buf_size, uint8_t audio_ch, void *const pcm_data,
			  uint16_t *const pcm_data_wr_size, bool bad_frame);

/**
 * @brief Initialize requested G.722 directions as one transaction.
 *
 * An already initialized opposite direction is preserved. Configuration is committed
 * only after all requested contexts and converters are ready. Independent configuration
 * and direction-state errors are reported before allocation; invalid configuration
 * takes precedence over an already initialized direction.
 *
 * @param[in]		cfg			Directions and fixed-profile codec parameters to
 *						initialize.
 * @param[in,out]	current			Non-NULL selector state; committed only on success.
 * @param[in,out]	encoder_converters	Non-NULL selector-owned array sized for
 *						CONFIG_AUDIO_ENCODE_CHANNELS_MAX contexts.
 * @param[in,out]	decoder_converters	Non-NULL selector-owned array sized for
 *						CONFIG_AUDIO_DECODE_CHANNELS_MAX contexts.
 *
 * @retval	0		Requested directions initialized.
 * @retval	-EINVAL		Invalid profile, channel count or empty direction selection.
 * @retval	-EALREADY	A requested direction is already initialized.
 * @return	Allocation or converter initialization error on failure; newly acquired
 *		contexts are released and existing directions remain unchanged.
 */
int sw_codec_g722_init(struct sw_codec_config cfg, struct sw_codec_config *current,
		       struct sample_rate_converter_ctx *encoder_converters,
		       struct sample_rate_converter_ctx *decoder_converters);

#endif /* SW_CODEC_G722_H_ */
