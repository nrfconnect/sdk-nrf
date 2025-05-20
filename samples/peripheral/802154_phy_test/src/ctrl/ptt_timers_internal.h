/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: timer interface intended for usage inside control module */

#ifndef PTT_TIMERS_INTERNAL_H__
#define PTT_TIMERS_INTERNAL_H__

#include "ctrl/ptt_timers.h"

#include "ptt_config.h"
#include "ptt_types.h"

/** timer */
struct ptt_timer_s {
	ptt_time_t timeout; /**< timeout, when expires user function will be called */
	ptt_timer_cb cb; /**< function to be called when timer is expired */
	ptt_evt_id_t evt_id; /**< event id which will be passed to user function */
	bool use; /**< flag to determine that timer currently at use */
};

/** context for timer */
struct ptt_timer_ctx_s {
	struct ptt_timer_s timer_pool[PTT_TIMER_POOL_N]; /**< array of timers */
	ptt_time_t last_update_time; /**< time of last update of used timers in the pool */
	ptt_time_t last_timeout; /**< timeout with which call_me_cb was called last time */
	uint8_t free_slots; /**< counter of free slots at timer_pool */
};

/** @brief Initialize timers context
 *
 *  @param none
 *
 *  @return none
 */
void ptt_timers_init(void);

/** @brief Uninitialize timers context
 *
 *  @param none
 *
 *  @return none
 */
void ptt_timers_uninit(void);

/** @brief Reset timers context to default values as after initialization
 *
 *  @param none
 *
 *  @return none
 */
void ptt_timers_reset_all(void);

/** @brief Find expired timers and calls their callbacks from `cb` field
 *
 *  @param current_time - current time, provided by an application, should be no more than max_time
 *
 *  @return none
 */
void ptt_timers_process(ptt_time_t current_time);

#endif /* PTT_TIMERS_INTERNAL_H__ */
