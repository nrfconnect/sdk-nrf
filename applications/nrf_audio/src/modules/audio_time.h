/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_app_time Audio Time
 * @{
 * @brief Audio time API for Audio applications.
 *
 * This module provides precise timing functionality for audio synchronization across
 * multiple devices.
 */

#ifndef _AUDIO_TIME_H_
#define _AUDIO_TIME_H_

#include <stdint.h>

/**
 * @brief Obtain the current audio time in microseconds.
 */
uint32_t audio_time_us_get(void);

/**
 * @}
 */

#endif /* _AUDIO_TIME_H_ */
