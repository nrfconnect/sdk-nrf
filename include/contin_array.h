/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONTIN_ARRAY_H
#define CONTIN_ARRAY_H

#include <zephyr/net_buf.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <audio_defines.h>

/**
 * @file contin_array.h
 *
 * @defgroup contin_array Continuous array
 * @{
 * @brief Basic continuous array.
 */

/** @brief Creates a continuous array from a finite array.
 *
 * @param pcm_cont		Pointer to the destination array.
 * @param pcm_cont_size	        Size of pcm_cont.
 * @param pcm_finite		Pointer to an array of samples or data.
 * @param pcm_finite_size	Size of pcm_finite.
 * @param finite_pos		Variable used internally. Must be set
 *				to 0 for the first run and not changed.
 *
 * @note  This function serves the purpose of e.g. having a set of audio samples
 * stored in pcm_finite. This can then be fetched in smaller pieces into ram and
 * played back in a loop using the results in pcm_cont.
 * The function keeps track of the current position in finite_pos,
 * so that the function can be called multiple times and maintain the correct
 * position in pcm_finite.
 *
 * @retval 0		If the operation was successful.
 * @retval -EPERM	If any sizes are zero.
 * @retval -ENXIO	On NULL pointer.
 */
int contin_array_create(void *pcm_cont, uint32_t pcm_cont_size, void const *const pcm_finite,
			uint32_t pcm_finite_size, uint32_t *const finite_pos);

/**
 * @brief Creates a continuous array in out_ch in the net_buf of channels, from a single channel
 * finite array.
 *
 * @note This function assumes that the continuos arrray buffer length is at least one sample per
 * channel.
 * @note This function assumes the finite array is of the same PCM format as the continuous array.
 *
 * @param pcm_contin       Pointer to the destination audio structure.
 * @param pcm_finite       Pointer to the finite array buffer.
 * @param pcm_finite_size  Size of the finite array buffer.
 * @param out_ch           Channel to write the finite array to.
 * @param finite_pos       Variable used internally. Must be set to 0 for the first run and not
 * changed.
 *
 * @note  This function serves the purpose of e.g. having a set of audio samples
 * stored in pcm_finite. This can then be fetched in smaller pieces into ram and
 * played back in a loop using the results in pcm_cont.
 * The function keeps track of the current position in finite_pos,
 * so that the function can be called multiple times and maintain the correct
 * position in pcm_finite.
 *
 * @retval 0        If the operation was successful.
 * @retval ENXIO    On NULL pointer.
 * @retval EPERM    If any sizes or out_ch is out of range.
 * @retval EINVAL   if the finite_pos is out of range.
 */
int contin_array_single_chan_buf_create(struct net_buf *pcm_contin, void const *const pcm_finite,
					uint16_t pcm_finite_size, uint8_t out_ch,
					uint16_t *const finite_pos);

/**
 * @brief Creates a continuous array for all channels a net_buf, from a single channel finite array.
 *
 * @note This function assumes that the continuous arrray buffer size is least one sample per
 * channel.
 * @note This function will update the continuous buffer length to match the number of channels
 * within the buffer multiplied by the bytes per channel.
 *
 * @param pcm_contin       Pointer to the destination audio structure.
 * @param pcm_finite       Pointer to the finite array array buffer.
 * @param pcm_finite_size  Size of the finite array buffer.
 * @param finite_pos       Variable used internally. Must be set to 0 for the first run and
 * not changed.
 *
 * @note  This function serves the purpose of e.g. having a set of audio samples
 * stored in pcm_finite. This can then be fetched in smaller pieces into ram and
 * played back in a loop using the results in pcm_cont.
 * The function keeps track of the current position in finite_pos,
 * so that the function can be called multiple times and maintain the correct
 * position in pcm_finite.
 *
 * @retval 0        If the operation was successful.
 * @retval EPERM    If any sizes are zero.
 * @retval ENXIO    On NULL pointer.
 * @retval EINVAL   if the finite_pos is out of range.
 */
int contin_array_chans_buf_create(struct net_buf *pcm_contin, void const *const pcm_finite,
				  uint16_t pcm_finite_size, uint16_t *const finite_pos);

/**
 * @brief Creates a continuous array for out_ch in a net_buf, from a single channel finite array in
 * a net_buf.
 *
 * @note This function assumes that the continuous arrray buffer length is at least one sample per
 * channel.
 *
 * @param pcm_contin       Pointer to the destination audio structure.
 * @param pcm_finite       Pointer to the finite array audio structure.
 * @param out_ch           Channel to write the finite array to.
 * @param finite_pos       Variable used internally. Must be set to 0 for the first run and not
 * changed.
 *
 * @note  This function serves the purpose of e.g. having a set of audio samples
 * stored in pcm_finite. This can then be fetched in smaller pieces into ram and
 * played back in a loop using the results in pcm_cont.
 * The function keeps track of the current position in finite_pos,
 * so that the function can be called multiple times and maintain the correct
 * position in pcm_finite.
 *
 * @retval 0        If the operation was successful.
 * @retval EPERM    If any sizes are zero or out_ch is out of range.
 * @retval ENXIO    On NULL pointer.
 * @retval EINVAL   if the finite_pos is out of range.
 */
int contin_array_single_chan_create(struct net_buf *pcm_contin, struct net_buf *pcm_finite,
				    uint8_t out_ch, uint16_t *const finite_pos);

/**
 * @brief Creates continuous array for all channels in a net_buf, from a single channel finite array
 * in a net_buf.
 *
 * @note This function assumes that the continuous arrray buffer size is least one sample per
 * channel.
 * @note This function will update the continuous buffer length to match the number of channels
 * within the buffer multiplied by the bytes per channel.
 *
 * @param pcm_contin       Pointer to the destination audio structure.
 * @param pcm_finite       Pointer to the finite array audio structure.
 * @param finite_pos       Variable used internally. Must be set to 0 for the first run and
 * not changed.
 *
 * @note  This function serves the purpose of e.g. having a set of audio samples
 * stored in pcm_finite. This can then be fetched in smaller pieces into ram and
 * played back in a loop using the results in pcm_cont.
 * The function keeps track of the current position in finite_pos,
 * so that the function can be called multiple times and maintain the correct
 * position in pcm_finite.
 *
 * @retval 0        If the operation was successful.
 * @retval EPERM    If any sizes are zero.
 * @retval ENXIO    On NULL pointer.
 * @retval EINVAL   if the finite_pos is out of range.
 */
int contin_array_chans_create(struct net_buf *pcm_contin, struct net_buf *pcm_finite,
			      uint16_t *const finite_pos);

/**
 * @}
 */
#endif /* CONTIN_ARRAY_H */
