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

/* Purpose: internal RF defines */

#ifndef PTT_RF_INTERNAL_H__
#define PTT_RF_INTERNAL_H__

#include <stdint.h>

#include "ptt_types.h"

#include "ctrl/ptt_events.h"

#include "rf/ptt_rf.h"

#define PTT_RF_EVT_UNLOCKED    (PTT_EVENT_UNASSIGNED)
#define PTT_RF_DEFAULT_CHANNEL (PTT_CHANNEL_MIN)
/* @todo: arbitrary selected, change PTT_RF_DEFAULT_POWER to appropriate value after testing */
#define PTT_RF_DEFAULT_POWER   (-20)
#define PTT_RF_DEFAULT_ANTENNA 0u

/** RF module context */
typedef struct
{
    uint8_t           channel;      /**< configured RF channel */
    int8_t            power;        /**< configured RF power */
    uint8_t           antenna;      /**< configured RF antenna */
    ptt_evt_id_t      evt_lock;     /**< current event processing by RF module */
    ptt_bool_t        cca_on_tx;    /**< perform CCA before transmission or not */
    ptt_bool_t        stat_enabled; /**< statistic gathering enabled */
    ptt_rf_stat_t     stat;         /**< received packets statistic */
    ptt_ltx_payload_t ltx_payload;  /**< raw payload for 'custom ltx' command */
} ptt_rf_ctx_t;

#endif  /* PTT_RF_INTERNAL_H__ */

