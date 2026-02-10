/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_clock Audio Clock
 * @{
 * @brief Audio clock management for nRF5340 Audio application.
 *
 * This module provides functions to manage the audio clock. On the nRF5340 SoC, the audio clock is
 * typically derived from the Analog Phase-Locked Loop (APLL), which is used to generate the
 * required frequencies for transmission, for instance I2S or TDM.
 */

#ifndef _AUDIO_CLOCK_H_
#define _AUDIO_CLOCK_H_

#include <stdint.h>
#include <nrfx_clock.h>

/** @brief HFCLKAUDIO frequency value for 12.288-MHz output.
 *
 * This macro defines the frequency value for the High Frequency Clock Audio (HFCLKAUDIO)
 * to generate a 12.288-MHz output frequency. The value is calculated according to the formula
 * in the nRF5340 SoC documentation.
 */
#define HFCLKAUDIO_12_288_MHZ 0x9BA6

#define HFCLKAUDIO_12_165_MHZ 0x8FD8

#define HFCLKAUDIO_12_411_MHZ 0xA774

/* Audio clock - nRF5340 Analog Phase-Locked Loop (APLL) */
#define APLL_FREQ_MIN	 HFCLKAUDIO_12_165_MHZ
#define APLL_FREQ_CENTER HFCLKAUDIO_12_288_MHZ
#define APLL_FREQ_MAX	 HFCLKAUDIO_12_411_MHZ

/**
 * @brief Set the audio clock frequency.
 * This function configures the audio clock to the specified frequency value. The frequency value
 * must be within the range defined by APLL_FREQ_MIN and APLL_FREQ_MAX. The function will adjust the
 * frequency to fit within this range if necessary.
 *
 * @param[in]	freq_value	The desired frequency value for the audio clock, calculated
 *				according to the formula in the nRF5340 SoC documentation.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int audio_clock_set(uint16_t freq_value);

/**
 * @brief Initialize the audio clock.
 * This function initializes the audio clock system, which may involve configuring the necessary
 * hardware components and ensuring that the clock is ready for use. This should be called before
 * any other audio clock functions are used.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int audio_clock_init(void);

/**
 * @}
 */

#endif /* _AUDIO_CLOCK_H_ */
