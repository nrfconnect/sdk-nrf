/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_CTRL_H
#define DECT_PHY_CTRL_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include "dect_common.h"
#include "dect_phy_common.h"
#include "dect_phy_shell.h"

#define DECT_PHY_CTRL_OP_DEBUG_ON			1
#define DECT_PHY_CTRL_OP_DEBUG_OFF			2

#define DECT_PHY_CTRL_OP_SETTINGS_UPDATED		6

#define DECT_PHY_CTRL_OP_PHY_API_MDM_COMPLETED		7
#define DECT_PHY_CTRL_OP_PHY_API_MDM_INITIALIZED	8
#define DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_DATA	9
#define DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC_CRC_ERROR	10
#define DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PDC_CRC_ERROR	11
#define DECT_PHY_CTRL_OP_PHY_API_MDM_RX_PCC		12

#define DECT_PHY_CTRL_OP_RSSI_SCAN_RUN			13
#define DECT_PHY_CTRL_OP_PHY_API_MDM_RSSI_CB_ON_RX_OP	14

#define DECT_PHY_CTRL_OP_PERF_CMD_DONE			15
#define DECT_PHY_CTRL_OP_PING_CMD_DONE			16
#define DECT_PHY_CTRL_OP_RF_TOOL_CMD_DONE		17

#define DECT_PHY_CTRL_OP_SCHEDULER_SUSPENDED		18
#define DECT_PHY_CTRL_OP_SCHEDULER_RESUMED		19

#define DECT_PHY_CTRL_OP_RADIO_ACTIVATED		20
#define DECT_PHY_CTRL_OP_RADIO_DEACTIVATED		21
#define DECT_PHY_CTRL_OP_RADIO_MODE_CONFIGURED		22

/******************************************************************************/

int dect_phy_ctrl_msgq_non_data_op_add(uint16_t event_id);
int dect_phy_ctrl_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size);

/******************************************************************************/

int dect_phy_ctrl_activate_cmd(enum nrf_modem_dect_phy_radio_mode radio_mode);
int dect_phy_ctrl_deactivate_cmd(void);
int dect_phy_ctrl_radio_mode_cmd(enum nrf_modem_dect_phy_radio_mode radio_mode);

/******************************************************************************/

void dect_phy_ctrl_status_get_n_print(void);

int dect_phy_ctrl_rx_start(struct dect_phy_rx_cmd_params *params, bool restart);
void dect_phy_ctrl_rx_stop(void);
bool dect_phy_ctrl_rx_is_ongoing(void);

int dect_phy_ctrl_current_radio_mode_get(
	enum nrf_modem_dect_phy_radio_mode *radio_mode);
int dect_phy_ctrl_time_query(void);

void dect_phy_ctrl_mdm_op_cancel_all(void);

int dect_phy_ctrl_modem_temperature_get(void);

uint64_t dect_phy_ctrl_modem_latency_for_next_op_get(bool is_tx);
uint64_t dect_phy_ctrl_modem_latency_min_margin_between_ops_get(void);

/******************************************************************************/

typedef void (*dect_phy_ctrl_rssi_scan_completed_callback_t)(
	enum nrf_modem_dect_phy_err phy_status);
int dect_phy_ctrl_rssi_scan_start(
	struct dect_phy_rssi_scan_params *params,
	dect_phy_ctrl_rssi_scan_completed_callback_t fp_result_callback);
void dect_phy_ctrl_rssi_scan_stop(void);

int dect_phy_ctrl_rssi_scan_results_print_and_best_channel_get(bool print_results);

/******************************************************************************/

bool dect_phy_ctrl_mdm_phy_api_initialized(void);
bool dect_phy_ctrl_phy_api_scheduler_suspended(void);

int8_t dect_phy_ctrl_utils_mdm_max_tx_pwr_dbm_get_by_channel(uint16_t channel);
uint8_t dect_phy_ctrl_utils_mdm_max_tx_phy_pwr_get_by_channel(uint16_t channel);
uint8_t dect_phy_ctrl_utils_mdm_next_supported_phy_tx_power_get(
	uint8_t phy_power, uint16_t channel);

/******************************************************************************/

int dect_phy_ctrl_perf_cmd(struct dect_phy_perf_params *params);
void dect_phy_ctrl_perf_cmd_stop(void);

/******************************************************************************/

int dect_phy_ctrl_ping_cmd(struct dect_phy_ping_params *params);
void dect_phy_ctrl_ping_cmd_stop(void);

/******************************************************************************/

int dect_phy_ctrl_rf_tool_cmd(struct dect_phy_rf_tool_params *params);
void dect_phy_ctrl_rf_tool_cmd_stop(void);

/******************************************************************************/

/* Calls external function if dect_phy_ctrl_ext_nbr_is_in_channel_cb_t is set
 * to get the nbr info for the given channel.
 * Returns false if no dect_phy_ctrl_ext_nbr_is_in_channel_cb_t set.
 */
bool dect_phy_ctrl_nbr_is_in_channel(uint16_t channel);

/******************************************************************************/

/* Callbacks that can be used by external commands */

/* Callbacks for handling modem events. Running in dect_phy_ctrl_th except named as "direct_" */

/* Callback for receiving PCC */
typedef void (*dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t)(
	struct dect_phy_common_op_pcc_rcv_params *pcc_params);
int dect_phy_ctrl_th_register_default_phy_api_pcc_rcv_cb(
	dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t default_pcc_rcv_cb);

/* Callback for receiving PDC */
typedef void (*dect_phy_ctrl_ext_phy_api_direct_pdc_rx_cb_t)(
	struct dect_phy_commmon_op_pdc_rcv_params *pdc_params);
typedef bool (*dect_phy_ctrl_ext_phy_api_pdc_rx_cb_t)(
	struct dect_phy_commmon_op_pdc_rcv_params *pdc_params);
int dect_phy_ctrl_th_register_default_phy_api_pdc_rcv_cb(
	dect_phy_ctrl_ext_phy_api_pdc_rx_cb_t default_pdc_rcv_cb);

/* Callback for receiving RSSI measurement results */
typedef void (*dect_phy_ctrl_ext_phy_api_direct_rssi_cb_t)(
	const struct nrf_modem_dect_phy_rssi_event *meas_results);

/* Callback for receiving modem operation complete events */
typedef void (*dect_phy_ctrl_ext_phy_api_mdm_op_complete_cb_t)(
	struct dect_phy_common_op_completed_params *params);

/******************************************************************************/
/* Callback for getting notified of settings change. Running in dect_phy_ctrl_th. */

typedef void (*dect_phy_ctrl_ext_settings_changed_cb_t)(void);

/******************************************************************************/

/* Register external function callback to request if the channel got any
 * neighbors on external neighbor list.
 * This information is used by RSSI scanning.
 */

typedef bool (*dect_phy_ctrl_ext_nbr_is_in_channel_cb_t)(uint16_t channel);
int dect_phy_ctrl_register_ext_nbr_is_in_channel_cb(
	dect_phy_ctrl_ext_nbr_is_in_channel_cb_t nbr_channel_cb);

struct dect_phy_ctrl_ext_callbacks {
	dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t direct_pcc_rcv_cb;
	dect_phy_ctrl_ext_phy_api_pcc_rx_cb_t pcc_rcv_cb;
	dect_phy_ctrl_ext_phy_api_direct_pdc_rx_cb_t direct_pdc_rcv_cb;
	dect_phy_ctrl_ext_phy_api_direct_rssi_cb_t direct_rssi_cb;
	dect_phy_ctrl_ext_phy_api_pdc_rx_cb_t pdc_rcv_cb;
	dect_phy_ctrl_ext_phy_api_mdm_op_complete_cb_t op_complete_cb;
	dect_phy_ctrl_ext_settings_changed_cb_t sett_changed_cb;
	dect_phy_ctrl_ext_nbr_is_in_channel_cb_t channel_has_nbr_cb;
};
int dect_phy_ctrl_ext_command_start(struct dect_phy_ctrl_ext_callbacks ext_callbacks);
void dect_phy_ctrl_ext_command_stop(void);

/******************************************************************************/

/* Common modem callbacks that can be handled by dect_phy_ctrl */
void dect_phy_ctrl_mdm_time_query_cb(
	const struct nrf_modem_dect_phy_time_get_event *evt, uint64_t *time);
void dect_phy_ctrl_mdm_radio_config_cb(
	const struct nrf_modem_dect_phy_radio_config_event *evt);
void dect_phy_ctrl_mdm_activate_cb(const struct nrf_modem_dect_phy_activate_event *evt);
void dect_phy_ctrl_mdm_deactivate_cb(const struct nrf_modem_dect_phy_deactivate_event *evt);
void dect_phy_ctrl_mdm_cancel_cb(const struct nrf_modem_dect_phy_cancel_event *evt);
void dect_phy_ctrl_mdm_configure_cb(const struct nrf_modem_dect_phy_configure_event *evt);
void dect_phy_ctrl_mdm_on_capability_get_cb(
	const struct nrf_modem_dect_phy_capability_get_event *evt);

#endif /* DECT_PHY_CTRL_H */
