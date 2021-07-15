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

/* Purpose: timer configuring and processing */

#include <kernel.h>

#include "timer_proc.h"

#include "ptt.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(timer);

static void ptt_process_timer_handler(struct k_timer * timer);
static void schedule_ptt_process(struct k_work * work);

K_TIMER_DEFINE(m_app_timer, ptt_process_timer_handler, NULL);

K_WORK_DEFINE(schedule_ptt_processor, schedule_ptt_process);

static void ptt_process_timer_handler(struct k_timer * timer)
{
    k_work_submit(&schedule_ptt_processor);
}

ptt_time_t ptt_get_current_time(void)
{
    return k_uptime_get_32();
}

ptt_time_t ptt_get_max_time(void)
{
    /* The maximum possible time is the maximum for a uint32_t */
    return UINT32_MAX;
}

void launch_ptt_process_timer(ptt_time_t timeout)
{
    if (0 == timeout)
    {
        /* schedule immediately */
        k_work_submit(&schedule_ptt_processor);
    }
    else
    {
        /* Schedule a single shot timer to trigger after the timeout */
        k_timer_start(&m_app_timer, K_MSEC(timeout), K_MSEC(0));
    }
}

/* scheduler context */
static void schedule_ptt_process(struct k_work * work)
{
    ptt_process(ptt_get_current_time());
}

