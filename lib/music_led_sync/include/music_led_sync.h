/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MUSIC_LED_SYNC_H_
#define _MUSIC_LED_SYNC_H_

#include <zephyr/kernel.h>

/**
 * @brief Update PCM data used for FFT
 *
 * @note PCM data gets copied to a local variable, where the copy get sent
 *       through CMSIS FFT. The results gets analyzed and the RGB LED
 *       changes accordingly.
 *
 * @param[in] temp_pcm_data_mono Pointer to PCM data (mono)
 * @param len Length of decoded data
 *
 * @return 0 on success, error otherwise
 */
int music_led_sync_data_update(char const *const temp_pcm_data_mono, uint16_t len);

/**
 * @brief Resumes music synchronization of RGB LED
 *
 */
void music_led_sync_play(void);

/**
 * @brief Suspends music synchronization of RGB LED
 *
 * @return 0 on success, error otherwise
 */
int music_led_sync_pause(void);

#endif /* _MUSIC_LED_SYNC_H */
