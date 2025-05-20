/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: events interface intended for usage inside control module */

#ifndef PTT_EVENTS_INTERNAL_H__
#define PTT_EVENTS_INTERNAL_H__

#include "ctrl/ptt_events.h"

#include "ptt_config.h"
#include "ptt_errors.h"
#include "ptt_types.h"
#include "ptt_utils.h"

/** events command type definition */
typedef uint32_t ptt_evt_cmd_t;

/** events state type definition */
typedef uint8_t ptt_evt_state_t;

PTT_COMPILE_TIME_ASSERT((PTT_EVENT_DATA_SIZE % sizeof(int32_t)) == 0u);

/** struct to hold packet data and length */
struct ptt_evt_data_s {
	uint8_t arr[PTT_EVENT_DATA_SIZE]; /**< packet */
	uint16_t len; /**< packet used lengths */
};

PTT_COMPILE_TIME_ASSERT((PTT_EVENT_CTX_SIZE % sizeof(int32_t)) == 0u);

/** struct to hold context data and length */
struct ptt_evt_ctx_data_s {
	uint8_t arr[PTT_EVENT_CTX_SIZE]; /**< context */
	uint16_t len; /**< context used lengths */
};

/** event data */
struct ptt_event_s {
	struct ptt_evt_data_s data; /**< packet payload */
	struct ptt_evt_ctx_data_s ctx; /**< command's context */
	ptt_evt_cmd_t cmd; /**< command */
	ptt_evt_state_t state; /**< command processing state */
	bool use; /**< flag to determine is this event currently in use */
};

/** events context */
struct ptt_evt_ctx_s {
	struct ptt_event_s evt_pool[PTT_EVENT_POOL_N]; /**< array of events */
};

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
 *  @param[out] evt_id - pointer to event index in event pool
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_event_alloc(ptt_evt_id_t *evt_id);

/** @brief Free event in the event pool
 *
 *  If event pointed by evt_id is free already returns PTT_RET_SUCCESS
 *
 *  @param evt_id - index of event in event pool
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_event_free(ptt_evt_id_t evt_id);

/** @brief Allocate an event and fill the event content with data provided by pkt and its length
 *
 *  Makes copy from pkt to the event content
 *
 *  @param[out] evt_id - index of event in event pool
 *  @param pkt - pointer to data
 *  @param len - length of data
 *
 *  @return PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_event_alloc_and_fill(ptt_evt_id_t *evt_id, const uint8_t *pkt, ptt_pkt_len_t len);

/** @brief Return pointer to an event by its index in the event pool
 *
 *  @param evt_id - index of event in event pool
 *
 *  @return pointer to the event, or NULL if the event is not found
 */
struct ptt_event_s *ptt_get_event_by_id(ptt_evt_id_t evt_id);

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
 *  @param data - pointer to data, mustn't be NULL
 *  @param len - length of the data, len <= PTT_EVENT_CTX_SIZE
 *
 *  @return none
 */
void ptt_event_set_ctx_data(ptt_evt_id_t evt_id, const uint8_t *data, uint16_t len);

/** @brief Returns a pointer to context data of the event
 *
 *  @param none
 *
 *  @return none
 */
struct ptt_evt_ctx_data_s *ptt_event_get_ctx_data(ptt_evt_id_t evt_id);

/** @brief Copy data to event data
 *
 *  @param evt_id - the event
 *  @param data - pointer to data, mustn't be NULL
 *  @param len - length of the data, len <= PTT_EVENT_DATA_SIZE
 *
 *  @return none
 */
void ptt_event_set_data(ptt_evt_id_t evt_id, const uint8_t *data, uint16_t len);

/** @brief Returns a pointer to data of the event
 *
 *  @param none
 *
 *  @return none
 */
struct ptt_evt_data_s *ptt_event_get_data(ptt_evt_id_t evt_id);

#endif /* PTT_EVENTS_INTERNAL_H__ */
