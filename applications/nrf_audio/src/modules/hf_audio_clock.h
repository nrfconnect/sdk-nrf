/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup hf_audio_clock High Frequency Audio Clock
 * @{
 * @brief Audio clock management for nRF Audio application.
 *
 * This module provides functions to manage the audio clock. On the nRF5340 SoC, the audio clock is
 * typically derived from the Analog Phase-Locked Loop (APLL), which is used to generate the
 * required frequencies for transmission, for instance I2S or TDM.
 */

#ifndef _HF_AUDIO_CLOCK_H_
#define _HF_AUDIO_CLOCK_H_

#include <stdint.h>
#include <nrfx_clock.h>

/**
 * @brief Update the audio clock frequency.
 * This function configures the audio clock to the specified frequency value. The frequency value
 * must be within the range defined by APLL_FREQ_MIN and APLL_FREQ_MAX. The function will adjust the
 * frequency to fit within this range if necessary.
 *
 * @param[in]	control_val_u	Pointer to the control value used to adjust the audio clock
 * frequency.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int hf_audio_clock_update(void *const control_val_u);

/**
 * @brief Initialize the audio clock.
 * This function initializes the audio clock system, which may involve configuring the necessary
 * hardware components and ensuring that the clock is ready for use. This should be called before
 * any other audio clock functions are used.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int hf_audio_clock_init(void);

/**
 * @}
 */

#endif /* _HF_AUDIO_CLOCK_H_ */
