/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONTIN_ARRAY_H
#define CONTIN_ARRAY_H

#include <zephyr/kernel.h>

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
 * @}
 */
#endif /* CONTIN_ARRAY_H */
