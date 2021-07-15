/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Purpose: timer interface intended for usage inside control module */

#ifndef PTT_TIMERS_INTERNAL_H__
#define PTT_TIMERS_INTERNAL_H__

#include "ctrl/ptt_timers.h"

#include "ptt_config.h"
#include "ptt_types.h"

/** timer */
typedef struct
{
    ptt_time_t   timeout; /**< timeout, when expires user function will be called */
    ptt_timer_cb cb;      /**< function to be called when timer is expired */
    ptt_evt_id_t evt_id;  /**< event id which will be passed to user function */
    ptt_bool_t   use;     /**< flag to determine that timer currently at use */
} ptt_timer_t;

/** context for timer */
typedef struct
{
    ptt_timer_t timer_pool[PTT_TIMER_POOL_N]; /**< array of timers */
    ptt_time_t  last_update_time;             /**< time of last update of used timers in the pool */
    ptt_time_t  last_timeout;                 /**< timeout with which call_me_cb was called last time */
    uint8_t     free_slots;                   /**< counter of free slots at timer_pool */
} ptt_timer_ctx_t;

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

