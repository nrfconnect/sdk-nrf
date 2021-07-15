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

/* Purpose: events implementation */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "ctrl/ptt_trace.h"

#include "ptt_events_internal.h"

#ifdef TESTS
#include "test_events_conf.h"
#else
#include "ptt_ctrl_internal.h"
#endif


void ptt_events_init(void)
{
    PTT_TRACE("ptt_events_init ->\n");

    ptt_evt_ctx_t * p_evt_ctx = ptt_ctrl_get_p_evt_ctx();

    *p_evt_ctx = (ptt_evt_ctx_t){0};

    PTT_TRACE("ptt_events_init -<\n");
}

void ptt_events_uninit(void)
{
    PTT_TRACE("ptt_events_uninit\n");
}

void ptt_events_reset_all(void)
{
    PTT_TRACE("ptt_events_reset_all ->\n");

    ptt_events_uninit();
    ptt_events_init();

    PTT_TRACE("ptt_events_reset_all -<\n");
}

ptt_ret_t ptt_event_alloc(ptt_evt_id_t * p_evt_id)
{
    PTT_TRACE("ptt_event_alloc -> p_evt %p\n", p_evt_id);

    uint8_t         i;
    ptt_evt_ctx_t * p_evt_ctx = ptt_ctrl_get_p_evt_ctx();
    ptt_ret_t       ret       = PTT_RET_SUCCESS;

    assert(p_evt_ctx != NULL);

    if (NULL == p_evt_id)
    {
        ret = PTT_RET_NULL_PTR;
        PTT_TRACE("ptt_event_alloc -< ret %d\n", ret);
        return ret;
    }

    for (i = 0; i < PTT_EVENT_POOL_N; ++i)
    {
        if (false == p_evt_ctx->evt_pool[i].use)
        {
            *p_evt_id                  = i;
            p_evt_ctx->evt_pool[i].use = true;

            ret = PTT_RET_SUCCESS;
            PTT_TRACE("ptt_event_alloc -< ret %d\n", ret);
            return ret;
        }
    }

    ret = PTT_RET_NO_FREE_SLOT;
    PTT_TRACE("ptt_event_alloc -< ret %d\n", ret);
    return ret;
}

ptt_ret_t ptt_event_free(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_event_free -> evt_id %d\n", evt_id);

    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (evt_id >= PTT_EVENT_POOL_N)
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("ptt_event_free -< ret %d\n", ret);
        return ret;
    }

    ptt_evt_ctx_t * p_evt_ctx = ptt_ctrl_get_p_evt_ctx();

    assert(p_evt_ctx != NULL);

    if (false == p_evt_ctx->evt_pool[evt_id].use)
    {
        /* evt already inactive */
        ret = PTT_RET_SUCCESS;
        PTT_TRACE("ptt_event_free -< ret %d\n", ret);
        return ret;
    }

    p_evt_ctx->evt_pool[evt_id] = (ptt_event_t){0};

    PTT_TRACE("ptt_event_free -< ret %d\n", ret);
    return ret;
}

ptt_ret_t ptt_event_alloc_and_fill(ptt_evt_id_t * p_evt_id, const uint8_t * p_pkt,
                                   ptt_pkt_len_t len)
{
    PTT_TRACE("ptt_event_alloc_and_fill -> p_evt %p p_pkt %p len %d\n", p_evt_id, p_pkt, len);

    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (NULL == p_pkt)
    {
        ret = PTT_RET_NULL_PTR;
        PTT_TRACE("ptt_event_alloc_and_fill -< ret %d\n", ret);
        return ret;
    }
    if (len > PTT_EVENT_DATA_SIZE)
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("ptt_event_alloc_and_fill -< ret %d\n", ret);
        return ret;
    }
    if (0 == len)
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("ptt_event_alloc_and_fill -< ret %d\n", ret);
        return ret;
    }

    ret = ptt_event_alloc(p_evt_id);

    if (PTT_RET_SUCCESS != ret)
    {
        PTT_TRACE("ptt_event_alloc_and_fill -< ret %d\n", ret);
        return ret;
    }

    ptt_event_t * p_event = ptt_get_p_event_by_id(*p_evt_id);

    assert(NULL != p_event);

    p_event->data.len = len;
    memcpy(p_event->data.arr, p_pkt, p_event->data.len);

    PTT_TRACE("ptt_event_alloc_and_fill -< ret %d\n", ret);
    return ret;
}

ptt_event_t * ptt_get_p_event_by_id(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_get_p_event_by_id -> evt_id %d\n", evt_id);

    if (evt_id >= PTT_EVENT_POOL_N)
    {
        return NULL;
    }

    ptt_evt_ctx_t * p_evt_ctx = ptt_ctrl_get_p_evt_ctx();

    assert(p_evt_ctx != NULL);

    if (false == p_evt_ctx->evt_pool[evt_id].use)
    {
        /* event inactive */
        return NULL;
    }

    return &p_evt_ctx->evt_pool[evt_id];
}

void ptt_event_set_cmd(ptt_evt_id_t evt_id, ptt_evt_cmd_t cmd)
{
    PTT_TRACE("ptt_event_set_cmd -> evt_id %d cmd %d\n", evt_id, cmd);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    p_event->cmd = cmd;

    PTT_TRACE("ptt_event_set_cmd -<\n");
}

ptt_evt_cmd_t ptt_event_get_cmd(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_event_get_cmd -> evt_id %d\n", evt_id);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    PTT_TRACE("ptt_event_get_cmd -< cmd %d\n", p_event->cmd);
    return p_event->cmd;
}

void ptt_event_set_state(ptt_evt_id_t evt_id, ptt_evt_state_t state)
{
    PTT_TRACE("ptt_event_set_state -> evt_id %d state %d\n", evt_id, state);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    p_event->state = state;

    PTT_TRACE("ptt_event_set_state -<\n");
}

ptt_evt_state_t ptt_event_get_state(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_event_get_state -> evt_id %d\n", evt_id);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    PTT_TRACE("ptt_event_get_state -< state %d\n", p_event->state);
    return p_event->state;
}

void ptt_event_set_ctx_data(ptt_evt_id_t evt_id, const uint8_t * p_data, uint16_t len)
{
    PTT_TRACE("ptt_event_set_data_context -> evt_id %d p_data %p len %d\n", evt_id, p_data, len);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    assert(p_data != NULL);
    assert(len <= PTT_EVENT_CTX_SIZE);

    p_event->ctx.len = len;
    memcpy(p_event->ctx.arr, p_data, p_event->ctx.len);

    PTT_TRACE("ptt_event_set_data_context -<\n");
}

ptt_evt_ctx_data_t * ptt_event_get_p_ctx_data(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_event_get_p_context_data -> evt_id %d\n", evt_id);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    PTT_TRACE("ptt_event_get_p_context_data -< ctx %p\n", &p_event->ctx);
    return &p_event->ctx;
}

void ptt_event_set_data(ptt_evt_id_t evt_id, const uint8_t * p_data, uint16_t len)
{
    PTT_TRACE("ptt_event_set_data -> evt_id %d p_data %p len %d\n", evt_id, p_data, len);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    assert(p_data != NULL);
    assert(len <= PTT_EVENT_DATA_SIZE);

    p_event->data.len = len;
    memcpy(p_event->data.arr, p_data, p_event->data.len);

    PTT_TRACE("ptt_event_set_data -<\n");
}

ptt_evt_data_t * ptt_event_get_p_data(ptt_evt_id_t evt_id)
{
    PTT_TRACE("ptt_event_get_p_data -> evt_id %d\n", evt_id);

    ptt_event_t * p_event = ptt_get_p_event_by_id(evt_id);

    assert(NULL != p_event);

    PTT_TRACE("ptt_event_get_p_data -< ctx %p\n", &p_event->data);
    return &p_event->data;
}

