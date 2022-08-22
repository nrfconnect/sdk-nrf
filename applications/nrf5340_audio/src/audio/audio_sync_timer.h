/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *
 *  SPDX-License-Identifier: LicenseRef-PCFT
 */

#ifndef _AUDIO_SYNC_TIMER_H_
#define _AUDIO_SYNC_TIMER_H_

#include <stdint.h>

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

#endif /* _AUDIO_SYNC_TIMER_H_ */
