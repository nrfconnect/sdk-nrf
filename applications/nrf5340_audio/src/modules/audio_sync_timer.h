/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_app_sync_timer Audio Sync Timer
 * @{
 * @brief Audio synchronization timer API for Audio applications.
 *
 * This module provides precise timing functionality for audio synchronization across
 * multiple devices.
 */

#ifndef _AUDIO_SYNC_TIMER_H_
#define _AUDIO_SYNC_TIMER_H_

#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * @brief Capture a timestamp on the sync timer.
 *
 * @retval The current timestamp of the audio sync timer.
 */
uint32_t audio_sync_timer_capture(void);

/**
 * @brief Returns the last captured value of the sync timer.
 *
 * The captured time is corresponding to the I2S frame start.
 * NOTE: This function is not reentrant and must only be called
 * once in the I2S ISR. There may be a delay in the capturing of
 * the clock value. Hence, there is a retry-loop with a timeout.
 * Should we not get the new capture value before the timeout,
 * a warning will be printed and calculations based on the old
 * timer capture values.
 *
 * See @ref audio_sync_timer_capture().
 *
 * @retval The last captured timestamp of the audio sync timer.
 */
uint32_t audio_sync_timer_frame_start_capture_get(void);

/**
 * @}
 */

#endif /* _AUDIO_SYNC_TIMER_H_ */
