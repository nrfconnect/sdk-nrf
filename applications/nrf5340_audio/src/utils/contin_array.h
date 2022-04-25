/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONTIN_ARRAY_H
#define CONTIN_ARRAY_H

#include <zephyr.h>

/** @brief. Creates a continuous array from a finite array
 * @param pcm_cont		Pointer to the destination array
 * @param pcm_cont_size	        Size of pcm_cont.
 * @param pcm_finite		Pointer to an array of samples/data
 * @param pcm_finite_size	Size of pcm_finite
 * @param finite_pos		Variable used interally. Must be set
 *				to 0 for first run and not changed.
 *
 * @note  This function serves the purpose of e.g. having a set of audio samples
 * stored in pcm_finite. This can then be fetched in smaller pieces into ram and
 * played back in a loop using the results in pcm_cont.
 *
 * @return 0 on success.        -EPERM if any sizes are zero
 *				-ENXIO on NULL pointer.
 */
int contin_array_create(void *pcm_cont, uint32_t pcm_cont_size, void const *const pcm_finite,
			uint32_t pcm_finite_size, uint32_t *const finite_pos);

#endif /* CONTIN_ARRAY_H */
