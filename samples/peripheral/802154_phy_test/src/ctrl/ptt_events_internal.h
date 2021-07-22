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

/* Purpose: events interface intended for usage inside control module */

#ifndef PTT_EVENTS_INTERNAL_H__
#define PTT_EVENTS_INTERNAL_H__

#include "ctrl/ptt_events.h"

#include "ptt_config.h"
#include "ptt_errors.h"
#include "ptt_types.h"
#include "ptt_types_internal.h"
#include "ptt_utils.h"

/** events command type definition */
typedef uint32_t ptt_evt_cmd_t;

/** events state type definition */
typedef uint8_t ptt_evt_state_t;

PTT_COMPILE_TIME_ASSERT((PTT_EVENT_DATA_SIZE % sizeof(int32_t)) == 0u);

/** struct to hold packet data and length */
typedef struct
{
    uint8_t  arr[PTT_EVENT_DATA_SIZE]; /**< packet */
    uint16_t len;                      /**< packet used lengths */
} ptt_evt_data_t;

PTT_COMPILE_TIME_ASSERT((PTT_EVENT_CTX_SIZE % sizeof(int32_t)) == 0u);

/** struct to hold context data and length */
typedef struct
{
    uint8_t  arr[PTT_EVENT_CTX_SIZE]; /**< context */
    uint16_t len;                     /**< context used lengths */
} ptt_evt_ctx_data_t;

/** event data */
typedef struct
{
    ptt_evt_data_t     data;  /**< packet payload */
    ptt_evt_ctx_data_t ctx;   /**< command's context */
    ptt_evt_cmd_t      cmd;   /**< command */
    ptt_evt_state_t    state; /**< command processing state */
    ptt_bool_t         use;   /**< flag to determine is this event currently in use */
} ptt_event_t;

/** events context */
typedef struct
{
    ptt_event_t evt_pool[PTT_EVENT_POOL_N]; /**< array of events */
} ptt_evt_ctx_t;

/** @brief Initialize events context
 *
 *  @param none
 *
 *  @return none
 */
void ptt_events_init(void);

/** @brief Uninitialize events context and free resources
 *
 *  @param none
 *
 *  @return none
 */
void ptt_events_uninit(void);

/** @brief Reset events context to default values as after initialization
 *
 *  @param none
 *
 *  @return none
 */
void ptt_events_reset_all(void);

/** @brief Allocate event entry in the event pool
 *
 *  @param[out] p_evt_id - pointer to event index in event pool
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_event_alloc(ptt_evt_id_t * p_evt_id);

/** @brief Free event in the event pool
 *
 *  If event pointed by evt_id is free already returns PTT_RET_SUCCESS
 *
 *  @param evt_id - index of event in event pool
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_event_free(ptt_evt_id_t evt_id);

/** @brief Allocate an event and fill the event content with data provided by p_pkt and its length
 *
 *  Makes copy from p_pkt to the event content
 *
 *  @param[out] evt_id - index of event in event pool
 *  @param p_pkt - pointer to data
 *  @param len - length of data
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
ptt_ret_t ptt_event_alloc_and_fill(ptt_evt_id_t * p_evt_id, const uint8_t * p_pkt,
                                   ptt_pkt_len_t len);

/** @brief Return pointer to an event by its index in the event pool
 *
 *  @param evt_id - index of event in event pool
 *
 *  @return pointer to the event, or NULL if the event is not found
 */
ptt_event_t * ptt_get_p_event_by_id(ptt_evt_id_t evt_id);

/** @brief Assign given command to the event
 *
 *  @param evt_id - the event
 *  @param cmd - command
 *
 *  @return none
 */
void ptt_event_set_cmd(ptt_evt_id_t evt_id, ptt_evt_cmd_t cmd);

/** @brief Get a command assigned to the event
 *
 *  @param evt_id - the even
 *
 *  @return ptt_evt_cmd_t Command
 */
ptt_evt_cmd_t ptt_event_get_cmd(ptt_evt_id_t evt_id);

/** @brief Assign given state to the event
 *
 *  @param evt_id - the event
 *  @param state - state
 *
 *  @return none
 */
void ptt_event_set_state(ptt_evt_id_t evt_id, ptt_evt_state_t state);

/** @brief Get a state assigned to the event
 *
 *  @param evt_id - the event
 *
 *  @return ptt_evt_state_t State
 */
ptt_evt_state_t ptt_event_get_state(ptt_evt_id_t evt_id);

/** @brief Copy data to event context
 *
 *  @param evt_id - the event
 *  @param p_data - pointer to data, mustn't be NULL
 *  @param len - length of the data, len <= PTT_EVENT_CTX_SIZE
 *
 *  @return none
 */
void ptt_event_set_ctx_data(ptt_evt_id_t evt_id, const uint8_t * p_data, uint16_t len);

/** @brief Returns a pointer to context data of the event
 *
 *  @param none
 *
 *  @return none
 */
ptt_evt_ctx_data_t * ptt_event_get_p_ctx_data(ptt_evt_id_t evt_id);

/** @brief Copy data to event data
 *
 *  @param evt_id - the event
 *  @param p_data - pointer to data, mustn't be NULL
 *  @param len - length of the data, len <= PTT_EVENT_DATA_SIZE
 *
 *  @return none
 */
void ptt_event_set_data(ptt_evt_id_t evt_id, const uint8_t * p_data, uint16_t len);

/** @brief Returns a pointer to data of the event
 *
 *  @param none
 *
 *  @return none
 */
ptt_evt_data_t * ptt_event_get_p_data(ptt_evt_id_t evt_id);

#endif /* PTT_EVENTS_INTERNAL_H__ */