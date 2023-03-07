/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief PCM audio mixer library header.
 */

#ifndef _PCM_MIX_H_
#define _PCM_MIX_H_

#include <zephyr/kernel.h>

/**
 * @defgroup pcm_mix Pulse Code Modulation mixer
 * @brief Pulse Code Modulation audio mixer library.
 *
 * @{
 */

enum pcm_mix_mode {
	B_STEREO_INTO_A_STEREO,
	B_MONO_INTO_A_MONO,
	B_MONO_INTO_A_STEREO_LR,
	B_MONO_INTO_A_STEREO_L,
	B_MONO_INTO_A_STEREO_R,
};

/**
 * @brief Mixes two buffers of PCM data.
 *
 * @note Uses simple addition with hard clip protection.
 * Input can be mono or stereo as long as the inputs match.
 * By selecting the mix mode, mono can also be mixed into a stereo buffer.
 * Hard coded for the signed 16-bit PCM.
 *
 * @param pcm_a         [in/out] Pointer to the PCM data buffer A.
 * @param size_a        [in]     Size of the PCM data buffer A (in bytes).
 * @param pcm_b         [in]     Pointer to the PCM data buffer B.
 * @param size_b        [in]     Size of the PCM data buffer B (in bytes).
 * @param mix_mode      [in]     Mixing mode according to pcm_mix_mode.
 *
 * @retval 0            Success. Result stored in pcm_a.
 * @retval -EINVAL      pcm_a is NULL or size_a = 0.
 * @retval -EPERM       Either size_b < size_a (for stereo to stereo, mono to mono)
 *			or size_a/2 < size_b (for mono to stereo mix).
 * @retval -ESRCH       Invalid mixing mode.
 */
int pcm_mix(void *const pcm_a, size_t size_a, void const *const pcm_b, size_t size_b,
	    enum pcm_mix_mode mix_mode);

/**
 * @}
 */
#endif /* _PCM_MIX_H_ */
