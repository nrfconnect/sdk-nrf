/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief PCM Stream Channel Modifier library header.
 */

#ifndef _PCM_STREAM_CHANNEL_MODIFIER_H_
#define _PCM_STREAM_CHANNEL_MODIFIER_H_

/**
 * @defgroup pscm PCM Stream Channel Modifier library
 * @brief Enables splitting of pulse-code modulation (PCM) streams from stereo to mono
 * or combine mono streams to form a stereo stream.
 *
 * @{
 */

#include <zephyr/kernel.h>
#include <audio_defines.h>

/** @brief  Adds a 0 after every sample from *input
 *	   and writes it to *output.
 * @note Use to create stereo stream from a mono source where one
 *	  channel is silent.
 *
 * @param[in]	input			Pointer to the input buffer.
 * @param[in]	input_size		Number of bytes in input.
 * @param[in]	channel			Channel to contain the audio data.
 * @param[in]	pcm_bit_depth		Bit depth of PCM samples (16, 24, or 32).
 * @param[out]	output			Pointer to the output buffer.
 * @param[out]	output_size		Number of bytes written to the output.
 *
 * @return	0 if success.
 */
int pscm_zero_pad(void const *const input, size_t input_size, enum audio_channel channel,
		  uint8_t pcm_bit_depth, void *output, size_t *output_size);

/** @brief  Adds a copy of every sample from *input
 *	   and writes it to both channels in *output.
 * @note Use to create stereo stream from a mono source where both
 *	  channels are identical.
 *
 * @param[in]	input			Pointer to the input buffer.
 * @param[in]	input_size		Number of bytes in input.
 * @param[in]	pcm_bit_depth		Bit depth of PCM samples (16, 24, or 32).
 * @param[out]	output			Pointer to the output buffer.
 * @param[out]	output_size		Number of bytes written to output.
 *
 * @return	0 if success.
 */
int pscm_copy_pad(void const *const input, size_t input_size, uint8_t pcm_bit_depth, void *output,
		  size_t *output_size);

/** @brief  Combines two mono streams into one stereo stream.
 *
 * @param[in]	input_left		Pointer to the input buffer for the left channel.
 * @param[in]	input_right		Pointer to the input buffer for the right channel.
 * @param[in]	input_size		Number of bytes in the input. Same for both channels.
 * @param[in]	pcm_bit_depth		Bit depth of PCM samples (16, 24, or 32).
 * @param[out]	output			Pointer to the output buffer.
 * @param[out]	output_size		Number of bytes written to the output.
 *
 * @return	0 if success.
 */
int pscm_combine(void const *const input_left, void const *const input_right, size_t input_size,
		 uint8_t pcm_bit_depth, void *output, size_t *output_size);

/** @brief  Removes every second sample from *input
 *	   and writes it to *output.
 * @note Use to split stereo audio stream to single channel.
 *
 * @param[in]	input			Pointer to the input buffer.
 * @param[in]	input_size		Number of bytes in the input. Must be
 *					divisible by two.
 * @param[in]	channel			Channel to keep the audio data from.
 * @param[in]	pcm_bit_depth		Bit depth of PCM samples (16, 24, or 32).
 * @param[out]	output			Pointer to the output buffer.
 * @param[out]	output_size		Number of bytes written to the output.
 *
 * @return	0 if success.
 */
int pscm_one_channel_split(void const *const input, size_t input_size, enum audio_channel channel,
			   uint8_t pcm_bit_depth, void *output, size_t *output_size);

/** @brief  Splits a stereo stream to two separate mono streams.
 * @note Use to split stereo audio stream to two separate channels.
 *
 * @param[in]	input			Pointer to the input buffer.
 * @param[in]	input_size		Number of bytes in input. Must be
 *					divisible by two.
 * @param[in]	pcm_bit_depth		Bit depth of PCM samples (16, 24, or 32).
 * @param[out]	output_left		Pointer to the output buffer containing
 *					the left channel.
 * @param[out]	output_right		Pointer to the output buffer containing
 *					the right channel.
 * @param[out]	output_size		Number of bytes written to the output,
 *					same for both channels.
 *
 * @return	0 if success.
 */
int pscm_two_channel_split(void const *const input, size_t input_size, uint8_t pcm_bit_depth,
			   void *output_left, void *output_right, size_t *output_size);

/**
 * @}
 */

#endif /* _PCM_STREAM_CHANNEL_MODIFIER_H_ */
