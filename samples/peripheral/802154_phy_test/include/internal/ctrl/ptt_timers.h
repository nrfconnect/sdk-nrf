/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: timers interface intended for internal usage on library level */

#ifndef PTT_TIMERS_H__
#define PTT_TIMERS_H__

#include "ptt_errors.h"
#include "ptt_events.h"
#include "ptt_types.h"

/** type definition of function to be call when timer is expired */
typedef void (*ptt_timer_cb)(ptt_evt_id_t evt_id);

/** @brief Add a timer to the timer pool
 *
 *  @param timeout - timeout for the timer in ms
 *  @param cb - function to be called when timeout expired
 *  @param evt_id - event id to identify timer also will be passed as parameter to user cb
 *
 *  @return zero on success, or error code
 */
enum ptt_ret ptt_timer_add(ptt_time_t timeout, ptt_timer_cb cb, ptt_evt_id_t evt_id);

/** @brief Free timer for evt_id
 *
 *  @param evt_id - event, corresponding to removing timer
 */
void ptt_timer_remove(ptt_evt_id_t evt_id);

#endif /* PTT_TIMERS_H__ */
