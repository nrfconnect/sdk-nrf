/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *
 *  SPDX-License-Identifier: LicenseRef-PCFT
 */

#ifndef _AUDIO_SYNC_TIMER_H_
#define _AUDIO_SYNC_TIMER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Get the I2S TX timestamp
 *
 * @return Captured value
 */
uint32_t audio_sync_timer_i2s_frame_start_ts_get(void);

/**
 * @brief Get the currrent timer value
 *
 * @return Captured value
 */
uint32_t audio_sync_timer_curr_time_get(void);

/**
 * @brief Set timer compare value for when to clear or set the sync LED
 *
 * @param compare_value New compare value
 * @param clear Clear or set the sync LED
 */
void audio_sync_timer_sync_led_cmpr_time_set(uint32_t compare_value, bool clear);

/**
 * @brief Turns on the LED configured as sync LED
 */
void audio_sync_timer_sync_led_on(void);

#endif /* _AUDIO_SYNC_TIMER_H_ */
