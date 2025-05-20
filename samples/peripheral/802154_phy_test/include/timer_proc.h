/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: timer interfaces */

#ifndef TIMER_PROC_H__
#define TIMER_PROC_H__

#include <stdint.h>

#include "ptt.h"

/** @brief Callback provided by application to be called from the library
 *         to schedule a timer with a given timeout
 *
 *  @param timeout - timeout in milliseconds
 *
 *  @return none
 */
void launch_ptt_process_timer(ptt_time_t timeout);

/** @brief Return maximum time supported by current timer implementation
 *
 *  @param none
 *
 *  @return ptt_time_t - maximum time in milliseconds
 */
ptt_time_t ptt_get_max_time(void);

#endif /* TIMER_PROC_H__ */
