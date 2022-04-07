/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PCM_MIX_H_
#define _PCM_MIX_H_

#include <zephyr.h>

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
 * Input can be mono or stereo as long as inputs match.
 * By selecting the mix mode, mono can also be mixed into a stereo buffer.
 * Hard coded for signed 16-bit PCM.
 *
 * @param pcm_a         [in/out]Pointer to buffer A PCM data
 * @param size_a        [in]    Size (bytes) of buffer A PCM data
 * @param pcm_b         [in]    Pointer to buffer B PCM data
 * @param size_b        [in]    Size (bytes) of buffer B PCM data
 * @param mix_mode      [in]    Mixing mode according to pcm_mix_mode
 *
 * @return 0            Success. Result stored in pcm_a
 * @return -EINVAL      pcm_a is NULL or size_a = 0
 * @return -EPERM       size_b < size_a for stereo to stereo, mono to mono
 *						or size_a/2 < size_b for mono to stereo mix
 * @return -ESRCH       Invalid mix_mode
 */
int pcm_mix(void *const pcm_a, size_t size_a, void const *const pcm_b, size_t size_b,
	    enum pcm_mix_mode mix_mode);

#endif /* _PCM_MIX_H_ */
