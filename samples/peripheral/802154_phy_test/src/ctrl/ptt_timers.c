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

/* Purpose: timers implementation */

#include <stddef.h>

#include "ptt.h"

#include "ctrl/ptt_trace.h"

#include "ptt_timers_internal.h"

#ifdef TESTS
#include "test_timers_conf.h"
#else
#include "ptt_ctrl_internal.h"
#endif

/* Main idea is to keep timeouts bounded to current time and refresh them every
   time when a new timer added or any timer is ended. The nearest timeout is
   selected from all timeouts in the pool and the application timer is called
   with this timeout. It helps to charge the application timer actually only
   when a timer which is closer than already charged one is added and allows
   to keep a pool of timers based on a single application timer.
 */

static void charge_app_timer(void);
static ptt_time_t get_min_timeout(void);
static void update_timers(ptt_time_t current_time);

static inline void app_timer_update(ptt_time_t timeout);
static inline ptt_time_t update_timeout(ptt_time_t timeout, ptt_time_t value);
static inline ptt_time_t time_diff(ptt_time_t prev_time, ptt_time_t new_time);

void ptt_timers_init(void)
{
    PTT_TRACE("ptt_timers_init ->\n");

    ptt_timer_ctx_t * p_timer_ctx = ptt_ctrl_get_p_timer_ctx();

    *p_timer_ctx                  = (ptt_timer_ctx_t){0};
    p_timer_ctx->free_slots       = PTT_TIMER_POOL_N;
    p_timer_ctx->last_timeout     = ptt_ctrl_get_max_time();
    p_timer_ctx->last_update_time = ptt_ctrl_get_max_time();

    PTT_TRACE("ptt_timers_init -<\n");
}

void ptt_timers_uninit(void)
{
    PTT_TRACE("ptt_timers_uninit\n");
}

void ptt_timers_reset_all(void)
{
    PTT_TRACE("ptt_timers_reset_all ->\n");
    ptt_timers_uninit();
    ptt_timers_init();
    PTT_TRACE("ptt_timers_reset_all -<\n");
}

static inline ptt_time_t time_diff(ptt_time_t prev_time, ptt_time_t new_time)
{
    if (prev_time <= new_time)
    {
        return (new_time - prev_time);
    }
    else
    {
        return ((ptt_ctrl_get_max_time() - prev_time) + new_time);
    }
}

static inline void app_timer_update(ptt_time_t timeout)
{
    PTT_TRACE("app_timer_update -> %d\n", timeout);

    ptt_timer_ctx_t * p_timer_ctx = ptt_ctrl_get_p_timer_ctx();

    p_timer_ctx->last_timeout = timeout;
    ptt_ctrl_call_me_cb(timeout);

    PTT_TRACE("app_timer_update -<\n");
}

static inline ptt_time_t update_timeout(ptt_time_t timeout, ptt_time_t value)
{
    if (value > timeout)
    {
        return 0;
    }
    else
    {
        return (timeout - value);
    }
}

void ptt_timers_process(ptt_time_t current_time)
{
    PTT_TRACE("ptt_timers_process -> %d\n", current_time);

    uint8_t           i;
    ptt_timer_ctx_t * p_timer_ctx;

    update_timers(current_time);

    p_timer_ctx = ptt_ctrl_get_p_timer_ctx();

    for (i = 0; i < PTT_TIMER_POOL_N; ++i)
    {
        if ((true == p_timer_ctx->timer_pool[i].use) && (0 == p_timer_ctx->timer_pool[i].timeout))
        {
            if (NULL != p_timer_ctx->timer_pool[i].cb)
            {
                p_timer_ctx->timer_pool[i].cb(p_timer_ctx->timer_pool[i].evt_id);
            }
            p_timer_ctx->timer_pool[i] = (ptt_timer_t){0};
            p_timer_ctx->free_slots++;
        }
    }

    charge_app_timer();

    PTT_TRACE("ptt_timers_process -<\n");
}

ptt_ret_t ptt_timer_add(ptt_time_t timeout, ptt_timer_cb cb, ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_timer_add -> tm %d cb %p evt %d\n", timeout, cb, evt_id);

    ptt_ret_t   ret   = PTT_RET_SUCCESS;
    ptt_timer_t timer = {0};

    if (timeout > ptt_ctrl_get_max_time())
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("ptt_timer_add -< ret %d\n", ret);
        return ret;
    }

    if (NULL == cb)
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("ptt_timer_add -< ret %d\n", ret);
        return ret;
    }
    else
    {
        timer.cb = cb;
    }

    if (evt_id >= PTT_EVENT_POOL_N)
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("ptt_timer_add -< ret %d\n", ret);
        return ret;
    }
    else
    {
        timer.evt_id = evt_id;
    }

    timer.timeout = timeout;
    timer.use     = true;

    ptt_timer_ctx_t * p_timer_ctx = ptt_ctrl_get_p_timer_ctx();

    if (0 == p_timer_ctx->free_slots)
    {
        ret = PTT_RET_NO_FREE_SLOT;
        PTT_TRACE("ptt_timer_add -< ret %d\n", ret);
        return ret;
    }

    ptt_time_t current_time = ptt_get_current_time();

    /* refresh last update time each time when the first timer is added to empty pool
     * to have actual time after large delay of update_timers calls */
    if (PTT_TIMER_POOL_N == p_timer_ctx->free_slots)
    {
        p_timer_ctx->last_update_time = current_time;

        /* as pool is empty */
        p_timer_ctx->timer_pool[0] = timer;
        p_timer_ctx->free_slots--;

        app_timer_update(timer.timeout);
    }
    else
    {
        /* need to bind all previously placed timers to current time to align them with
           timer of adding new timer */
        /* legend:
            o - time of last binding timers to absolute time (last_update_time)
            c - current time (time of adding the new timer)
            a - time when the application timer will shoot
            t0, t1 - previously added timers
            t2 - new timer
            x0, x1, x2 - timeouts
         */
        /* o----c-----a
         |-------------x0    t0
         |----------x1       t1
           .....|..........    t2(new)
         */
        update_timers(current_time);
        /* o----c-----a
           .....|--------x0    t0
           .....|-----x1       t1
           .....|..........    t2(new)
         */

        /* add timer to pool */
        for (uint8_t i = 0; i < PTT_TIMER_POOL_N; ++i)
        {
            if (false == p_timer_ctx->timer_pool[i].use)
            {
                p_timer_ctx->timer_pool[i] = timer;
                p_timer_ctx->free_slots--;
                break;
            }
        }
        /* o----c-----a
           .....|--------x0    t0
           .....|-----x1       t1
           .....|----------x2  t2(new)
         */

        /* check is new timer closer than already charged in the application timer */
        /* note: timer.timeout = x2 - c */
        /* note: p_timer_ctx->last_timeout = x1 - c */
        if (p_timer_ctx->last_timeout > timer.timeout)
        {
            app_timer_update(timer.timeout);
        }
    }

    PTT_TRACE("ptt_timer_add -< ret %d\n", ret);
    return ret;
}

/* if removing timer was going to expire then one of next calls from an application
   will be ptt_timers_process, where timer pool will be updated and new application
   timer will be charged if any timers are running
 */
void ptt_timer_remove(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_timer_remove -> evt %d\n", evt_id);

    uint8_t           i;
    ptt_timer_ctx_t * p_timer_ctx = ptt_ctrl_get_p_timer_ctx();

    for (i = 0; i < PTT_TIMER_POOL_N; ++i)
    {
        if (evt_id == p_timer_ctx->timer_pool[i].evt_id)
        {
            p_timer_ctx->timer_pool[i] = (ptt_timer_t){0};
            p_timer_ctx->free_slots   += 1;
        }
    }

    PTT_TRACE("ptt_timer_remove -<\n");
}

static void charge_app_timer(void)
{
    PTT_TRACE("charge_app_timer ->\n");

    ptt_time_t current_time = ptt_get_current_time();

    update_timers(current_time);
    ptt_time_t nearest_timeout = get_min_timeout();

    app_timer_update(nearest_timeout);

    PTT_TRACE("charge_app_timer -<\n");
}

static ptt_time_t get_min_timeout(void)
{
    PTT_TRACE("get_min_timeout ->\n");

    /* @fixme: division by 4 is workaround to update application timer variables
               min_timeout = ptt_ctrl_get_max_time() should be after bug is fixed */
    ptt_time_t        min_timeout = ptt_ctrl_get_max_time() / 4;
    ptt_timer_ctx_t * p_timer_ctx = ptt_ctrl_get_p_timer_ctx();
    uint8_t           i;

    for (i = 0; i < PTT_TIMER_POOL_N; ++i)
    {
        if ((true == p_timer_ctx->timer_pool[i].use) &&
            (min_timeout > p_timer_ctx->timer_pool[i].timeout))
        {
            min_timeout = p_timer_ctx->timer_pool[i].timeout;
        }
    }

    PTT_TRACE("get_min_timeout -< tm %d\n", min_timeout);
    return min_timeout;
}

static void update_timers(ptt_time_t current_time)
{
    PTT_TRACE("update_timers -> ct %d\n", current_time);

    uint8_t           i;
    ptt_timer_ctx_t * p_timer_ctx = ptt_ctrl_get_p_timer_ctx();

    /* calculate time difference */
    ptt_time_t diff = time_diff(p_timer_ctx->last_update_time, current_time);

    for (i = 0; i < PTT_TIMER_POOL_N; ++i)
    {
        if (true == p_timer_ctx->timer_pool[i].use)
        {
            p_timer_ctx->timer_pool[i].timeout = update_timeout(p_timer_ctx->timer_pool[i].timeout,
                                                                diff);
        }
    }

    p_timer_ctx->last_timeout     = update_timeout(p_timer_ctx->last_timeout, diff);
    p_timer_ctx->last_update_time = current_time;

    PTT_TRACE("update_timers -<\n");
}