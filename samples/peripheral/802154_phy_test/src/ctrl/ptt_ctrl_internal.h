/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: control module interface intended for internal usage on library
 *          level
 */

#ifndef PTT_CTRL_INTERNAL_H__
#define PTT_CTRL_INTERNAL_H__

#include "ctrl/ptt_ctrl.h"

#include "ptt_modes.h"
#include "ptt_events_internal.h"
#include "ptt_timers_internal.h"

/** type definition of function to handle event */
typedef void (*ptt_ext_evt_handler)(ptt_evt_id_t evt_id);

/** handlers for each event
 *  NULL if unsupported by current mode
 */
struct ptt_ext_evts_handlers_s {
	ptt_ext_evt_handler rf_tx_finished; /**< packet transmission finished */
	ptt_ext_evt_handler rf_tx_failed; /**< packet transmission failed */
	ptt_ext_evt_handler rf_tx_started; /**< packet transmission started */
	ptt_ext_evt_handler rf_rx_done; /**< packet received over radio */
	ptt_ext_evt_handler rf_rx_failed; /**< packet reception failed */
	ptt_ext_evt_handler rf_cca_done; /**< CCA procedure finished */
	ptt_ext_evt_handler rf_cca_failed; /**< CCA procedure failed */
	ptt_ext_evt_handler rf_ed_detected; /**< ED procedure finished */
	ptt_ext_evt_handler rf_ed_failed; /**< ED procedure failed */
	ptt_ext_evt_handler uart_pkt_received; /**< packet received over UART */
};

/** control module context */
struct ptt_ctrl_s {
	ptt_call_me_cb_t call_me_cb;
	/**< callback from application to be call when need to update nearest timeout */
	enum ptt_mode_t current_mode; /**< current device mode */
	struct ptt_ext_evts_handlers_s mode_handlers; /**< event handlers */
	struct ptt_timer_ctx_s timer_ctx; /**< timers context */
	struct ptt_evt_ctx_s evt_ctx; /**< events context */
	ptt_time_t rsp_timeout; /**< common timeout for all responses */
	ptt_time_t max_time; /**< maximum time supported by application timer */
};

/* Get timer context */
struct ptt_timer_ctx_s *ptt_ctrl_get_timer_ctx(void);

/* Get event context */
struct ptt_evt_ctx_s *ptt_ctrl_get_evt_ctx(void);

/* Call call_me_cb - an application callback with given timeout */
void ptt_ctrl_call_me_cb(ptt_time_t timeout);

/* Get response timeout */
ptt_time_t ptt_ctrl_get_rsp_timeout(void);

/* Set response timeout */
void ptt_ctrl_set_rsp_timeout(ptt_time_t timeout);

/* Get max time */
ptt_time_t ptt_ctrl_get_max_time(void);

/* Get SoC temperature */
enum ptt_ret ptt_ctrl_get_temp(int32_t *temp);

/* Get current mode */
enum ptt_mode_t ptt_ctrl_get_current_mode(void);

/* Set current mode */
void ptt_ctrl_set_current_mode(enum ptt_mode_t mode);

/* Get default mode */
enum ptt_mode_t ptt_ctrl_get_default_mode(void);

/* Get HW version */
uint8_t ptt_ctrl_get_hw_version(void);

/* Get SW version */
uint8_t ptt_ctrl_get_sw_version(void);

/* Get external events handlers structure */
struct ptt_ext_evts_handlers_s *ptt_ctrl_get_handlers(void);

/* Disable/enable DC/DC mode */
void ptt_ctrl_set_dcdc(bool activate);

/* Get current DC/DC mode */
bool ptt_ctrl_get_dcdc(void);

/* Enable/Disable ICACHE */
void ptt_ctrl_set_icache(bool enable);

/* Get current state of ICACHE */
bool ptt_ctrl_get_icache(void);

/* Reset external events structure */
void ptt_ctrl_handlers_reset_all(void);

/* Get current handler for a packet received through UART */
ptt_ext_evt_handler ptt_ctrl_get_handler_uart_pkt_received(void);

/* Get current handler for Transmission finished event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_finished(void);

/* Get current handler for Transmission failed event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_failed(void);

/* Get current handler for Reception failed event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_failed(void);

/* Get current handler for Transmission started event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_started(void);

/* Get current handler for a packet received through RF */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_done(void);

/* Get current handler for CCA done procedure */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_done(void);

/* Get current handler for CCA failed procedure */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_failed(void);

/* Get current handler for ED done procedure*/
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_detected(void);

/* Get current handler for ED failed procedure */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_failed(void);

/** Get vector of random values
 *
 *  @param buff         - buffer to store vector, must be at least requested_size long
 *  @param requested_size - number of random bytes to store in buff
 *
 *  @return - None
 */
void ptt_random_vector_generate(uint8_t *buff, uint8_t requested_size);

/* Change state of LED indicating received packet to ON */
void ptt_ctrl_led_indication_on(void);

/* Change state of LED indicating received packet to OFF */
void ptt_ctrl_led_indication_off(void);

#endif /* PTT_CTRL_INTERNAL_H__ */
