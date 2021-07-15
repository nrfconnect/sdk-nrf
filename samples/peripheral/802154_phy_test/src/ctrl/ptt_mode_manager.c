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

/* Purpose: mode manager implementation. */

#include "ptt.h"

#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"

#include "ptt_ctrl_internal.h"
#include "ptt_modes.h"


static ptt_ret_t ptt_mode_init(ptt_mode_t mode);

/* caller should take care about freeing shared resources */
ptt_ret_t ptt_mode_default_init(void)
{
    ptt_mode_t mode = ptt_ctrl_get_default_mode();

    return ptt_mode_init(mode);
}

ptt_ret_t ptt_mode_switch(ptt_mode_t new_mode)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;
    uint32_t  mode_mask;

    if (new_mode >= PTT_MODE_N)
    {
        PTT_TRACE("ptt_mode_switch: invalid new_mode %d\n", new_mode);
        ret = PTT_RET_INVALID_VALUE;
    }
    else
    {
        if (ptt_get_mode_mask_ext(&mode_mask))
        {
            /* if new_mode isn't allowed */
            if ((mode_mask & (1u << new_mode)) == 0)
            {
                PTT_TRACE("ptt_mode_switch: new_mode isn't allowed %d\n", new_mode);
                ret = PTT_RET_INVALID_VALUE;
            }
        }
    }

    if (PTT_RET_SUCCESS == ret)
    {
        ret = ptt_mode_uninit();
        if (PTT_RET_SUCCESS == ret)
        {
            ret = ptt_mode_init(new_mode);
        }
    }

    return ret;
}

ptt_ret_t ptt_mode_uninit(void)
{
    ptt_mode_t current_mode = ptt_ctrl_get_current_mode();
    ptt_ret_t  ret          = PTT_RET_SUCCESS;

    switch (current_mode)
    {
        case PTT_MODE_ZB_PERF_DUT:
            ret = ptt_zb_perf_dut_mode_uninit();
            break;

        case PTT_MODE_ZB_PERF_CMD:
            ret = ptt_zb_perf_cmd_mode_uninit();
            break;

        default:
            PTT_TRACE("ptt_mode_uninit: invalid current_mode %d\n", current_mode);
            ret = PTT_RET_INVALID_MODE;
            break;
    }

    if (PTT_RET_SUCCESS == ret)
    {
        ptt_timers_reset_all();
        ptt_events_reset_all();
        ptt_ctrl_handlers_reset_all();
        ptt_rf_reset();
        ptt_ctrl_set_current_mode(PTT_MODE_N);
    }

    return ret;
}

/* caller should take care about freeing shared resources */
static ptt_ret_t ptt_mode_init(ptt_mode_t mode)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    switch (mode)
    {
        case PTT_MODE_ZB_PERF_DUT:
            ret = ptt_zb_perf_dut_mode_init();
            break;

        case PTT_MODE_ZB_PERF_CMD:
            ret = ptt_zb_perf_cmd_mode_init();
            break;

        default:
            PTT_TRACE("ptt_mode_init: invalid mode %d\n", mode);
            ret = PTT_RET_INVALID_MODE;
            break;
    }

    if (PTT_RET_SUCCESS == ret)
    {
        ptt_ctrl_set_current_mode(mode);
    }

    return ret;
}

