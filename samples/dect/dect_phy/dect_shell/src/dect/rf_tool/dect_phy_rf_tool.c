/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>

#include <dk_buttons_and_leds.h>
#include <nrf_modem_dect_phy.h>

#include "desh_defines.h"
#include "desh_print.h"
#include "dect_phy_common_rx.h"
#include "dect_common_utils.h"
#include "dect_phy_api_scheduler.h"

#include "dect_common_settings.h"
#include "dect_phy_ctrl.h"

#include "dect_phy_rf_tool.h"

K_SEM_DEFINE(rf_tool_phy_api_init, 0, 1);

struct dect_phy_rf_tool_tx_metrics {
	uint32_t tx_total_data_amount;
	uint32_t tx_total_pkt_count;
	uint32_t tx_op_to_mdm_failure_count;
	uint32_t tx_op_in_mdm_failure_count;
	uint32_t tx_total_mdm_operations;
	int64_t tx_last_tx_scheduled_frame_time_mdm_ticks;
};
struct dect_phy_rf_tool_rx_metrics {
	uint64_t last_pcc_stf_time;
	uint64_t last_synch_mdm_time;

	bool rx_wait_for_synch;

	uint64_t first_rx_mdm_op_frame_time;

	uint32_t rx_total_data_amount;
	uint32_t rx_total_pkt_count;

	uint32_t rx_op_to_mdm_ok_count;
	uint32_t rx_op_to_mdm_failure_count;
	uint32_t rx_op_in_mdm_failure_count;
	uint32_t rx_total_mdm_ok_operations;

	uint32_t rx_pcc_crc_error_count;
	uint32_t rx_pcc_header_not_valid_count;

	uint32_t rx_pdc_crc_error_count;

	int8_t rx_rssi_high_level;
	int8_t rx_rssi_low_level;
	uint8_t rx_phy_transmit_pwr_low;
	uint8_t rx_phy_transmit_pwr_high;
	int16_t rx_snr_low;
	int16_t rx_snr_high;
	uint8_t rx_latest_mcs;
};

struct dect_phy_rf_tool_scheduler_rx_data {
	struct nrf_modem_dect_phy_rx_params rx_op;
};

struct dect_phy_rf_tool_scheduler_tx_data {
	uint16_t tx_data_len;
	uint8_t tx_data[DECT_DATA_MAX_LEN]; /* max data size in bytes when MCS4 + 4 slots */

	union nrf_modem_dect_phy_hdr tx_phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
};

static struct dect_phy_rf_tool_tool_data {

	uint32_t frame_repeat_count_interval_current_count;
	uint32_t frame_count_for_synch;
	uint16_t frame_len_subslots;
	uint64_t frame_len_mdmticks;

	bool on_going;

	struct dect_phy_rf_tool_params cmd_params;

	struct dect_phy_rf_tool_scheduler_rx_data scheduler_rx_data;
	struct dect_phy_rf_tool_scheduler_tx_data scheduler_tx_data;

	struct dect_phy_rf_tool_rx_metrics rx_metrics;
	struct dect_phy_rf_tool_tx_metrics tx_metrics;
} rf_tool_data;

/**************************************************************************************************/

#define DECT_PHY_RF_TOOL_RESTART_SCHEDULING_DELAY_SECS 2

/**************************************************************************************************/
static int dect_phy_rf_tool_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size);
static int dect_phy_rf_tool_msgq_non_data_op_add(uint16_t event_id);

static int dect_phy_rf_tool_frame_active_subslots_get(enum dect_phy_rf_tool_mode mode);

static int dect_phy_rf_tool_frame_len_subslots_get(enum dect_phy_rf_tool_mode mode);
static uint64_t dect_phy_rf_tool_frame_len_mdmticks_get(enum dect_phy_rf_tool_mode mode);
static uint64_t dect_phy_rf_tool_next_frame_time_get(bool restart);

static int dect_phy_rf_tool_operations_schedule(uint64_t init_frame_start_time);

static int dect_phy_rf_tool_start(struct dect_phy_rf_tool_params *params, bool restart);
static const char *dect_phy_rf_tool_test_mode_string_get(enum dect_phy_rf_tool_mode mode);

static bool dect_phy_rf_tool_rx_mode(enum dect_phy_rf_tool_mode mode);

/**************************************************************************************************/

#define DECT_PHY_RF_TOOL_EVT_CMD_DONE			    1
#define DECT_PHY_RF_TOOL_EVT_MDM_OP_COMPLETED		    2
#define DECT_PHY_RF_TOOL_EVT_RX_PCC_CRC_ERROR		    3
#define DECT_PHY_RF_TOOL_EVT_RX_PDC_CRC_ERROR		    4
#define DECT_PHY_RF_TOOL_EVT_RX_PCC			    5
#define DECT_PHY_RF_TOOL_EVT_RX_PDC_DATA		    6
#define DECT_PHY_RF_TOOL_EVT_SCHEDULER_ALL_REPETITIONS_DONE 7

/**************************************************************************************************/

static void dect_phy_rf_tool_mdm_op_complete_cb(
	const struct nrf_modem_dect_phy_op_complete_event *evt,
	uint64_t *time)
{
	struct dect_phy_common_op_completed_params rf_tool_op_completed_params = {
		.handle = evt->handle,
		.temperature = evt->temp,
		.status = evt->err,
		.time = *time,
	};

	dect_phy_api_scheduler_mdm_op_completed(&rf_tool_op_completed_params);
	dect_phy_rf_tool_msgq_data_op_add(DECT_PHY_RF_TOOL_EVT_MDM_OP_COMPLETED,
					  (void *)&rf_tool_op_completed_params,
					  sizeof(struct dect_phy_common_op_completed_params));
}

static void dect_phy_rf_tool_mdm_pcc_cb(const struct nrf_modem_dect_phy_pcc_event *evt,
	uint64_t *time)
{
	struct dect_phy_common_op_pcc_rcv_params ctrl_pcc_op_params;
	struct dect_phy_header_type2_format0_t *header = (void *)&evt->hdr;

	ctrl_pcc_op_params.pcc_status = *evt;
	ctrl_pcc_op_params.phy_header = evt->hdr;
	ctrl_pcc_op_params.time = *time;
	ctrl_pcc_op_params.stf_start_time = evt->stf_start_time;
	ctrl_pcc_op_params.phy_len = header->packet_length;
	ctrl_pcc_op_params.phy_len_type = header->packet_length_type;

	dect_phy_rf_tool_msgq_data_op_add(DECT_PHY_RF_TOOL_EVT_RX_PCC,
					  (void *)&ctrl_pcc_op_params,
					  sizeof(struct dect_phy_common_op_pcc_rcv_params));
}

static void dect_phy_rf_tool_mdm_pcc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pcc_crc_failure_event *evt,
	uint64_t *time)
{
	struct dect_phy_common_op_pcc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_rf_tool_msgq_data_op_add(DECT_PHY_RF_TOOL_EVT_RX_PCC_CRC_ERROR,
					  (void *)&pdc_crc_fail_params,
					  sizeof(struct dect_phy_common_op_pcc_crc_fail_params));
}

static void dect_phy_rf_tool_mdm_pdc_cb(const struct nrf_modem_dect_phy_pdc_event *evt,
	uint64_t *time)
{
	int16_t rssi_level = evt->rssi_2 / 2;
	struct dect_phy_commmon_op_pdc_rcv_params rf_tool_pdc_op_params;

	rf_tool_pdc_op_params.rx_status = *evt;

	rf_tool_pdc_op_params.data_length = evt->len;
	rf_tool_pdc_op_params.time = *time;

	rf_tool_pdc_op_params.rx_pwr_dbm = 0;		      /* Taken from PCC */
	rf_tool_pdc_op_params.rx_rssi_level_dbm = rssi_level; /* Used from PCC */
	rf_tool_pdc_op_params.rx_channel = rf_tool_data.cmd_params.channel;

	if (evt->len <= sizeof(rf_tool_pdc_op_params.data)) {
		memcpy(rf_tool_pdc_op_params.data, evt->data, evt->len);
		dect_phy_rf_tool_msgq_data_op_add(
			DECT_PHY_RF_TOOL_EVT_RX_PDC_DATA, (void *)&rf_tool_pdc_op_params,
			sizeof(struct dect_phy_commmon_op_pdc_rcv_params));
	} else {
		printk("Received data is too long to be received by PERF TH - discarded (len %d, "
		       "buf size %d)\n",
		       evt->len, sizeof(rf_tool_pdc_op_params.data));
	}
}

static void dect_phy_rf_tool_mdm_pdc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt,
	uint64_t *time)
{
	struct dect_phy_common_op_pdc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_rf_tool_msgq_data_op_add(DECT_PHY_RF_TOOL_EVT_RX_PDC_CRC_ERROR,
					  (void *)&pdc_crc_fail_params,
					  sizeof(struct dect_phy_common_op_pdc_crc_fail_params));
}

/**************************************************************************************************/
static bool dect_phy_rf_tool_rx_mode(enum dect_phy_rf_tool_mode mode)
{
	return (mode == DECT_PHY_RF_TOOL_MODE_RX || mode == DECT_PHY_RF_TOOL_MODE_RX_TX ||
		mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);
}

static int dect_phy_rf_tool_rx_pdc_data_handle(struct dect_phy_data_rcv_common_params *params)
{
	rf_tool_data.rx_metrics.rx_total_data_amount += params->data_length;
	rf_tool_data.rx_metrics.rx_total_pkt_count++;

	return 0;
}

static double dect_phy_rf_tool_rx_n_tx_duty_cycle_percent_calculate(void)
{
	/* Calculate actual Duty Cycle */
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	int active_tx_subslot_count_on_frame =
		dect_phy_rf_tool_frame_active_subslots_get(DECT_PHY_RF_TOOL_MODE_TX);
	int active_rx_subslot_count_on_frame =
		dect_phy_rf_tool_frame_active_subslots_get(DECT_PHY_RF_TOOL_MODE_RX);
	int total_actual_active_tx_subslot_count_on_frames =
		MIN(rf_tool_data.tx_metrics.tx_total_mdm_operations,
		    cmd_params->frame_repeat_count) *
		active_tx_subslot_count_on_frame;
	int total_actual_active_rx_subslot_count_on_frames =
		MIN(rf_tool_data.rx_metrics.rx_total_mdm_ok_operations,
		    cmd_params->frame_repeat_count) *
		active_rx_subslot_count_on_frame;
	int frame_len_subslots = dect_phy_rf_tool_frame_len_subslots_get(cmd_params->mode);
	int interval_frames_subslots_count = cmd_params->frame_repeat_count * frame_len_subslots;
	double duty_cycle_percent = 0;

	if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_TX) {
		duty_cycle_percent = (double)total_actual_active_tx_subslot_count_on_frames /
				     interval_frames_subslots_count * 100;
	} else if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX) {
		duty_cycle_percent = (double)total_actual_active_rx_subslot_count_on_frames /
				     interval_frames_subslots_count * 100;
	} else if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		duty_cycle_percent = (double)(total_actual_active_tx_subslot_count_on_frames +
					      total_actual_active_rx_subslot_count_on_frames) /
				     interval_frames_subslots_count * 100;
	} else {
		int active_subslot_count_on_frame = dect_phy_rf_tool_frame_active_subslots_get(
			DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);

		__ASSERT_NO_MSG(cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);
		if (active_subslot_count_on_frame == 0) {
			return 0;
		}
		total_actual_active_rx_subslot_count_on_frames =
			cmd_params->frame_repeat_count * active_subslot_count_on_frame;
		duty_cycle_percent = (double)total_actual_active_rx_subslot_count_on_frames /
				     interval_frames_subslots_count * 100;
	}
	return duty_cycle_percent;
}

void dect_phy_rf_tool_print_results(void)
{
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	double duty_cycle_percent = dect_phy_rf_tool_rx_n_tx_duty_cycle_percent_calculate();

	desh_print("RF tool results at transmitter id %d:",
		   current_settings->common.transmitter_id);

	if (duty_cycle_percent == 0) {
		desh_print("  - RX/TX Duty Cycle percentage:                 N/A");
	} else {
		desh_print("  - RX/TX Duty Cycle percentage:                 %.02f%%",
			   duty_cycle_percent);
	}

	if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_TX ||
	    cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		desh_print("  - TX: packet count:                            %d",
			   rf_tool_data.tx_metrics.tx_total_pkt_count);
		desh_print("  - TX: data amount:                             %d",
			   rf_tool_data.tx_metrics.tx_total_data_amount);
		desh_print("  - TX: successfully completed modem operations: %d",
			   rf_tool_data.tx_metrics.tx_total_mdm_operations);
		desh_print("  - TX: failed operation count sent to modem:    %d",
			   rf_tool_data.tx_metrics.tx_op_to_mdm_failure_count);
		desh_print("  - TX: operation count failed in modem:         %d",
			   rf_tool_data.tx_metrics.tx_op_in_mdm_failure_count);
	}

	if (dect_phy_rf_tool_rx_mode(cmd_params->mode)) {

		uint32_t pkts_total =
			cmd_params->frame_repeat_count - rf_tool_data.frame_count_for_synch;
		int32_t pkts_missing = cmd_params->frame_repeat_count -
				       rf_tool_data.rx_metrics.rx_total_pkt_count -
				       rf_tool_data.frame_count_for_synch;

		if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS &&
		    cmd_params->peer_mode == DECT_PHY_RF_TOOL_MODE_NONE) {
			pkts_missing = 0;
		}

		desh_print("  - RX: packet count:                            %d",
			   rf_tool_data.rx_metrics.rx_total_pkt_count);
		desh_print("  - RX: data amount:                             %d",
			   rf_tool_data.rx_metrics.rx_total_data_amount);
		desh_print("  - RX: frame count used for synch:              %d",
			   rf_tool_data.frame_count_for_synch);
		desh_print("  - RX: successfully completed modem operations: %d",
			   rf_tool_data.rx_metrics.rx_total_mdm_ok_operations);
		desh_print("  - RX: failed operation count sent to modem:    %d",
			   rf_tool_data.rx_metrics.rx_op_to_mdm_failure_count);
		desh_print("  - RX: operation count failed in modem:         %d",
			   rf_tool_data.rx_metrics.rx_op_in_mdm_failure_count);

		if (pkts_missing > 0) {
			double packet_error_rate = (double)pkts_missing / pkts_total * 100;

			desh_error("  - RX: packets missing:                         %d",
				   pkts_missing);
			desh_error("  - RX: Packet Error Rate:                       %.02f%% "
				   "(=%d/%d*100)",
				   packet_error_rate, pkts_missing, pkts_total);
		}

		if (rf_tool_data.rx_metrics.rx_pcc_header_not_valid_count > 0) {
			desh_error("  - RX: PCC header not valid count:              %d",
				   rf_tool_data.rx_metrics.rx_pcc_header_not_valid_count);
		} else {
			desh_print("  - RX: PCC header not valid count:              %d",
				   rf_tool_data.rx_metrics.rx_pcc_header_not_valid_count);
		}

		if (rf_tool_data.rx_metrics.rx_pcc_crc_error_count > 0) {
			desh_error("  - RX: PCC CRC errors:                          %d",
				   rf_tool_data.rx_metrics.rx_pcc_crc_error_count);
		} else {
			desh_print("  - RX: PCC CRC errors:                          %d",
				   rf_tool_data.rx_metrics.rx_pcc_crc_error_count);
		}

		if (rf_tool_data.rx_metrics.rx_pdc_crc_error_count > 0) {
			desh_error("  - RX: PDC CRC errors:                          %d",
				   rf_tool_data.rx_metrics.rx_pdc_crc_error_count);
		} else {
			desh_print("  - RX: PDC CRC errors:                          %d",
				   rf_tool_data.rx_metrics.rx_pdc_crc_error_count);
		}

		desh_print("  - RX: RSSI high level:                         %d",
			   rf_tool_data.rx_metrics.rx_rssi_high_level);
		desh_print("  - RX: RSSI low level:                          %d",
			   rf_tool_data.rx_metrics.rx_rssi_low_level);

		desh_print("  - RX: transmit power high:                     %d dbm",
			   dect_common_utils_phy_tx_power_to_dbm(
				   rf_tool_data.rx_metrics.rx_phy_transmit_pwr_high));
		desh_print("  - RX: transmit power low:                      %d dbm",
			   dect_common_utils_phy_tx_power_to_dbm(
				   rf_tool_data.rx_metrics.rx_phy_transmit_pwr_low));

		desh_print("  - RX SNR high:                                 %d",
			   rf_tool_data.rx_metrics.rx_snr_high);
		desh_print("  - RX SNR low:                                  %d",
			   rf_tool_data.rx_metrics.rx_snr_low);
		desh_print("  - RX latest MCS:                               %d",
			   rf_tool_data.rx_metrics.rx_latest_mcs);
	}
}

static void dect_phy_rf_tool_cmd_done(void)
{
	dect_phy_rf_tool_print_results();

	/* Mdm phy api deinit is done by dect_phy_ctrl */
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_RF_TOOL_CMD_DONE);

	rf_tool_data.on_going = false;
}

/**************************************************************************************************/

K_MSGQ_DEFINE(dect_phy_rf_tool_op_event_msgq, sizeof(struct dect_phy_common_op_event_msgq_item),
	      500, /* After mdm deinit() there will come bunch of unhandled rx/tx responses with
		    * high repeat_count values.
		    */
	      4);

/**************************************************************************************************/

#define DECT_PHY_CERT_THREAD_STACK_SIZE 4096
#define DECT_PHY_CERT_THREAD_PRIORITY	5

static void dect_phy_rf_tool_thread_fn(void)
{
	struct dect_phy_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_rf_tool_op_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_PHY_RF_TOOL_EVT_CMD_DONE: {
			dect_phy_rf_tool_cmd_done();
			break;
		}
		case DECT_PHY_RF_TOOL_EVT_MDM_OP_COMPLETED: {
			struct dect_phy_common_op_completed_params *params =
				(struct dect_phy_common_op_completed_params *)event.data;

			if (rf_tool_data.on_going) {
				if (DECT_PHY_RF_TOOL_TX_HANDLE_IN_RANGE(params->handle)) {
					if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
						rf_tool_data.tx_metrics.tx_total_pkt_count++;
						rf_tool_data.tx_metrics.tx_total_data_amount +=
							rf_tool_data.scheduler_tx_data.tx_data_len;
						rf_tool_data.tx_metrics.tx_total_mdm_operations++;
					} else {
						rf_tool_data.tx_metrics
							.tx_op_in_mdm_failure_count++;
					}
				} else if (params->handle == DECT_PHY_RF_TOOL_RX_CONT_HANDLE ||
					   params->handle ==
						   DECT_PHY_RF_TOOL_SYNCH_WAIT_RX_HANDLE ||
					   DECT_PHY_RF_TOOL_RX_HANDLE_IN_RANGE(params->handle)) {
					if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
						rf_tool_data.rx_metrics
							.rx_total_mdm_ok_operations++;
					} else {
						rf_tool_data.rx_metrics
							.rx_op_in_mdm_failure_count++;
					}
				}
				if (rf_tool_data.rx_metrics.rx_wait_for_synch &&
				    params->handle == DECT_PHY_RF_TOOL_SYNCH_WAIT_RX_HANDLE) {
					/* We didn't got synch - stop */
					desh_print("No RX synch - exiting.");
					dect_phy_rf_tool_msgq_non_data_op_add(
						DECT_PHY_RF_TOOL_EVT_CMD_DONE);
				} else if (params->handle == DECT_PHY_RF_TOOL_RX_CONT_HANDLE) {
					dect_phy_rf_tool_msgq_data_op_add(
						DECT_PHY_RF_TOOL_EVT_SCHEDULER_ALL_REPETITIONS_DONE,
						&params->handle, sizeof(uint32_t));
				}
			}
			break;
		}
		case DECT_PHY_RF_TOOL_EVT_RX_PCC_CRC_ERROR: {
			if (rf_tool_data.on_going) {
				rf_tool_data.rx_metrics.rx_pcc_crc_error_count++;
			}

			break;
		}
		case DECT_PHY_RF_TOOL_EVT_RX_PDC_CRC_ERROR: {
			if (rf_tool_data.on_going) {
				rf_tool_data.rx_metrics.rx_pdc_crc_error_count++;
			}
			break;
		}
		case DECT_PHY_RF_TOOL_EVT_RX_PCC: {
			struct dect_phy_common_op_pcc_rcv_params *params =
				(struct dect_phy_common_op_pcc_rcv_params *)event.data;
			int16_t rssi_level = params->pcc_status.rssi_2 / 2;
			struct dect_phy_ctrl_field_common *phy_h = (void *)&(params->phy_header);
			struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);

			/* Store min/max RSSI/SNR/tx pwr as seen on PCC RX */
			if (rssi_level > rf_tool_data.rx_metrics.rx_rssi_high_level) {
				rf_tool_data.rx_metrics.rx_rssi_high_level = rssi_level;
			}
			if (rssi_level < rf_tool_data.rx_metrics.rx_rssi_low_level) {
				rf_tool_data.rx_metrics.rx_rssi_low_level = rssi_level;
			}
			if (phy_h->transmit_power >
			    rf_tool_data.rx_metrics.rx_phy_transmit_pwr_high) {
				rf_tool_data.rx_metrics.rx_phy_transmit_pwr_high =
					phy_h->transmit_power;
			}
			if (phy_h->transmit_power <
			    rf_tool_data.rx_metrics.rx_phy_transmit_pwr_low) {
				rf_tool_data.rx_metrics.rx_phy_transmit_pwr_low =
					phy_h->transmit_power;
			}

			if (params->pcc_status.snr > rf_tool_data.rx_metrics.rx_snr_high) {
				rf_tool_data.rx_metrics.rx_snr_high = params->pcc_status.snr;
			}
			if (params->pcc_status.snr < rf_tool_data.rx_metrics.rx_snr_low) {
				rf_tool_data.rx_metrics.rx_snr_low = params->pcc_status.snr;
			}

			rf_tool_data.rx_metrics.rx_latest_mcs = phy_h->df_mcs;
			rf_tool_data.rx_metrics.last_pcc_stf_time = params->stf_start_time;

			if (params->pcc_status.header_status !=
			    NRF_MODEM_DECT_PHY_HDR_STATUS_VALID) {
				rf_tool_data.rx_metrics.rx_pcc_header_not_valid_count++;
				break;
			}
			if (!rf_tool_data.on_going) {
				desh_error("Unexpected RX PCC - no RF tool running");
				break;
			}

			if (cmd_params->find_rx_sync) {
				if (rf_tool_data.rx_metrics.rx_total_pkt_count == 0) {
					/* 1st pkt, we got synch, now schedule operations ASAP  */
					rf_tool_data.rx_metrics.last_synch_mdm_time =
						params->stf_start_time;

					if (rf_tool_data.rx_metrics.rx_wait_for_synch) {
						rf_tool_data.rx_metrics.rx_wait_for_synch = false;

						uint64_t next_possible_frame_time = 0;

						next_possible_frame_time =
							dect_phy_rf_tool_next_frame_time_get(false);
						dect_phy_rf_tool_operations_schedule(
							next_possible_frame_time);
					}
				} else if (rf_tool_data.rx_metrics.rx_total_pkt_count == 1) {
					/* 1st pkt was for rx sync and now we need to calculate how
					 * many frames there were between this one and the 1st
					 * packet.
					 */
					rf_tool_data.frame_count_for_synch =
						((params->stf_start_time -
						  rf_tool_data.rx_metrics.last_synch_mdm_time) /
						 rf_tool_data.frame_len_mdmticks) -
						1;
				}
			} else {
				if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX &&
				    rf_tool_data.rx_metrics.rx_total_pkt_count == 0) {
					/* Synch is from our 1st RX frame time sent to modem */
					rf_tool_data.frame_count_for_synch =
						(params->stf_start_time -
						 rf_tool_data.rx_metrics
							 .first_rx_mdm_op_frame_time) /
						rf_tool_data.frame_len_mdmticks;
				}
			}
			break;
		}
		case DECT_PHY_RF_TOOL_EVT_RX_PDC_DATA: {
			struct dect_phy_commmon_op_pdc_rcv_params *params =
				(struct dect_phy_commmon_op_pdc_rcv_params *)event.data;
			struct dect_phy_data_rcv_common_params rcv_params = {
				.time = params->time,
				.snr = params->rx_status.snr,
				.rssi = params->rx_status.rssi_2 / 2,
				.rx_rssi_dbm = params->rx_rssi_level_dbm,
				.rx_pwr_dbm = params->rx_pwr_dbm,
				.data_length = params->data_length,
				.data = params->data,
			};

			if (params->data_length) {
				if (rf_tool_data.on_going) {
					dect_phy_rf_tool_rx_pdc_data_handle(&rcv_params);
				} else {
					desh_print("RF tool command TX data received: ignored "
						   "- no command running.");
				}
			} else {
				desh_error("RX PDC data with 0 length received - ignored.");
			}
			break;
		}
		case DECT_PHY_RF_TOOL_EVT_SCHEDULER_ALL_REPETITIONS_DONE: {
			uint32_t handle = *((uint32_t *)event.data);
			bool last_interval_done_event = false;
			struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);

			if (rf_tool_data.on_going) {
				desh_print("All repetitions done for handle %d", handle);
				if (DECT_PHY_RF_TOOL_TX_HANDLE_IN_RANGE(handle)) {
					last_interval_done_event = true;
				} else if (DECT_PHY_RF_TOOL_RX_HANDLE_IN_RANGE(handle) &&
					   cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX) {
					last_interval_done_event = true;
				} else if (handle == DECT_PHY_RF_TOOL_RX_CONT_HANDLE &&
					   cmd_params->mode ==
						   DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS) {
					last_interval_done_event = true;
				}
				if (last_interval_done_event) {
					rf_tool_data.frame_repeat_count_interval_current_count++;

					if (rf_tool_data.cmd_params.continuous ||
					    rf_tool_data.frame_repeat_count_interval_current_count <
						    rf_tool_data.cmd_params
							    .frame_repeat_count_intervals) {
						/* Continuous mode: schedule next iteration ASAP */
						dect_phy_rf_tool_print_results();
						dect_phy_rf_tool_start(&rf_tool_data.cmd_params,
								       true);
					} else {
						/* Single shot mode: done */
						dect_phy_rf_tool_msgq_non_data_op_add(
							DECT_PHY_RF_TOOL_EVT_CMD_DONE);
					}
				}
			}
			break;
		}

		default:
			desh_warn("DECT RF TOOL: Unknown event %d received", event.id);
			break;
		}
		k_free(event.data);
	}
}

K_THREAD_DEFINE(dect_phy_rf_tool_th, DECT_PHY_CERT_THREAD_STACK_SIZE, dect_phy_rf_tool_thread_fn,
		NULL, NULL, NULL, K_PRIO_PREEMPT(DECT_PHY_CERT_THREAD_PRIORITY), 0, 0);

/**************************************************************************************************/

static uint64_t dect_phy_rf_tool_next_possible_restart_frame_time_get(void)
{
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_frame_time =
		time_now + (SECONDS_TO_MODEM_TICKS(DECT_PHY_RF_TOOL_RESTART_SCHEDULING_DELAY_SECS));
	uint64_t next_frame_start;
	uint64_t frame_len = rf_tool_data.frame_len_mdmticks;

	if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_TX) {
		next_frame_start =
			rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks;
		while (next_frame_start < first_possible_frame_time) {
			next_frame_start += frame_len;
		}
	} else if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX ||
		   cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		if (cmd_params->find_rx_sync) {
			uint64_t offset = cmd_params->rx_frame_start_offset *
					  DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;
			uint64_t last_rx = rf_tool_data.rx_metrics.last_pcc_stf_time;

			__ASSERT_NO_MSG(last_rx != 0);

			next_frame_start = last_rx - offset;
		} else {
			next_frame_start =
				rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks;
		}
		while (next_frame_start < first_possible_frame_time) {
			next_frame_start += frame_len;
		}
	} else {
		__ASSERT_NO_MSG(cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);
		next_frame_start = 0; /* asap */
	}
	return next_frame_start;
}

static uint64_t dect_phy_rf_tool_next_frame_time_get(bool restart)
{
	if (restart) {
		return dect_phy_rf_tool_next_possible_restart_frame_time_get();
	}

	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_start =
		time_now + (US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));
	uint64_t next_possible_frame_time = 0;
	uint64_t frame_len = rf_tool_data.frame_len_mdmticks;

	if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_TX) {
		uint64_t last_tx =
			rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks;
		uint64_t last_frame_start_time = last_tx;
		uint64_t next_frame_start = last_frame_start_time;

		while (next_frame_start < first_possible_start) {
			next_frame_start += frame_len;
		}
		next_possible_frame_time = next_frame_start;
	} else if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX ||
		   cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		uint64_t last_rx = rf_tool_data.rx_metrics.last_pcc_stf_time;
		uint64_t offset = cmd_params->rx_frame_start_offset *
				  DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;
		uint64_t last_frame_start_time = last_rx - offset;
		uint64_t next_frame_start = last_frame_start_time;

		while (next_frame_start < first_possible_start) {
			next_frame_start += frame_len;
		}
		next_possible_frame_time = next_frame_start;
	} else {
		__ASSERT_NO_MSG(rf_tool_data.cmd_params.mode ==
				DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);
		next_possible_frame_time = first_possible_start;
	}
	return next_possible_frame_time;
}

static int dect_phy_rf_tool_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);

	event.id = event_id;
	ret = k_msgq_put(&dect_phy_rf_tool_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

static int dect_phy_rf_tool_msgq_non_data_op_add(uint16_t event_id)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;
	k_timeout_t timeout = K_NO_WAIT;

	event.id = event_id;
	event.data = NULL;

	ret = k_msgq_put(&dect_phy_rf_tool_op_event_msgq, &event, timeout);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

void dect_phy_rf_tool_evt_handler(const struct nrf_modem_dect_phy_event *evt)
{
	dect_app_modem_time_save(&evt->time);

	switch (evt->id) {
	case NRF_MODEM_DECT_PHY_EVT_PCC:
		dect_phy_rf_tool_mdm_pcc_cb(&evt->pcc, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC_ERROR:
		dect_phy_rf_tool_mdm_pcc_crc_failure_cb(&evt->pcc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC:
		dect_phy_rf_tool_mdm_pdc_cb(&evt->pdc, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC_ERROR:
		dect_phy_rf_tool_mdm_pdc_crc_failure_cb(&evt->pdc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_COMPLETED:
		dect_phy_rf_tool_mdm_op_complete_cb(&evt->op_complete, (uint64_t *)&evt->time);
		break;

	/* Callbacks handled by dect_phy_ctrl */
	case NRF_MODEM_DECT_PHY_EVT_RADIO_CONFIG:
		dect_phy_ctrl_mdm_radio_config_cb(&evt->radio_config);
		break;
	case NRF_MODEM_DECT_PHY_EVT_ACTIVATE:
		dect_phy_ctrl_mdm_activate_cb(&evt->activate);
		break;
	case NRF_MODEM_DECT_PHY_EVT_DEACTIVATE:
		dect_phy_ctrl_mdm_deactivate_cb(&evt->deactivate);
		break;
	case NRF_MODEM_DECT_PHY_EVT_CANCELED:
		dect_phy_ctrl_mdm_cancel_cb(&evt->cancel);
		break;

	default:
		printk("%s: WARN: Unexpected event id %d\n", (__func__), evt->id);
		break;
	}
}

static void dect_phy_rf_tool_phy_init(void)
{
	int ret = nrf_modem_dect_phy_event_handler_set(dect_phy_rf_tool_evt_handler);

	if (ret) {
		printk("nrf_modem_dect_phy_event_handler_set returned: %i\n", ret);
	}
}

/**************************************************************************************************/

static void dect_phy_rf_tool_rx_to_mdm_cb(
	struct dect_phy_common_op_completed_params *params, uint64_t frame_time)
{
	if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
		if (rf_tool_data.rx_metrics.rx_op_to_mdm_ok_count == 0) {
			rf_tool_data.rx_metrics.first_rx_mdm_op_frame_time = frame_time;
		}
		rf_tool_data.rx_metrics.rx_op_to_mdm_ok_count++;
	} else {
		rf_tool_data.rx_metrics.rx_op_to_mdm_failure_count++;
	}
}

static void dect_phy_rf_tool_tx_to_mdm_cb(
	struct dect_phy_common_op_completed_params *params, uint64_t frame_time)
{
	if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
		rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks = frame_time;
		rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks = frame_time;
	} else {
		rf_tool_data.tx_metrics.tx_op_to_mdm_failure_count++;
	}
}

static void dect_phy_rf_tool_all_intervals_done(uint32_t handle)
{
	__ASSERT_NO_MSG(DECT_PHY_RF_TOOL_TX_HANDLE_IN_RANGE(handle) ||
			DECT_PHY_RF_TOOL_RX_HANDLE_IN_RANGE(handle));
	dect_phy_rf_tool_msgq_data_op_add(DECT_PHY_RF_TOOL_EVT_SCHEDULER_ALL_REPETITIONS_DONE,
					  &handle, sizeof(uint32_t));
}

static int dect_phy_rf_tool_rx_op_schedule(uint64_t frame_time, uint32_t interval_mdm_ticks)
{
	struct dect_phy_api_scheduler_list_item_config *list_item_conf;
	struct dect_phy_api_scheduler_list_item *list_item =
		dect_phy_api_scheduler_list_item_alloc_rx_element(&list_item_conf);
	int ret = 0;

	if (!list_item) {
		desh_error("Failed to allocate list item for RX operation");
		return -ENOMEM;
	}
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	list_item_conf->cb_op_to_mdm = dect_phy_rf_tool_rx_to_mdm_cb;
	list_item_conf->cb_op_completed = NULL;
	list_item_conf->cb_op_completed_with_count = dect_phy_rf_tool_all_intervals_done;

	list_item_conf->interval_count_left = cmd_params->frame_repeat_count;
	list_item_conf->interval_mdm_ticks = interval_mdm_ticks;
	list_item_conf->channel = cmd_params->channel;
	list_item_conf->frame_time = frame_time;
	list_item_conf->start_slot = 0;
	list_item_conf->start_subslot = cmd_params->rx_frame_start_offset;

	list_item_conf->subslot_used = true;
	list_item_conf->length_slots = 0;
	list_item_conf->length_subslots = cmd_params->rx_subslot_count;
	list_item_conf->rx.duration = 0;

	list_item->priority = DECT_PRIORITY1_RX;
	list_item->phy_op_handle = DECT_PHY_RF_TOOL_RX_HANDLE_START;
	list_item_conf->phy_op_handle_range_start = DECT_PHY_RF_TOOL_RX_HANDLE_START;
	list_item_conf->phy_op_handle_range_end = DECT_PHY_RF_TOOL_RX_HANDLE_END;
	list_item_conf->phy_op_handle_range_used = true;
	list_item_conf->rx.mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT;
	list_item_conf->rx.expected_rssi_level = cmd_params->expected_rx_rssi_level;

	if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS) {
		list_item_conf->length_subslots = 0;
		list_item_conf->rx.duration = UINT32_MAX;

		/* Set peer parameters defines reporting interval for rx_cont mode */
		if (cmd_params->peer_mode != DECT_PHY_RF_TOOL_MODE_NONE) {
			list_item_conf->rx.duration =
				(rf_tool_data.frame_len_mdmticks * cmd_params->frame_repeat_count) +
				(SECONDS_TO_MODEM_TICKS(
					DECT_PHY_RF_TOOL_RESTART_SCHEDULING_DELAY_SECS / 3));
		}

		list_item_conf->interval_count_left = 0;
		list_item_conf->interval_mdm_ticks = 0;
		list_item_conf->cb_op_to_mdm_with_interval_count_completed = NULL;
		list_item_conf->cb_op_completed_with_count = NULL;
		list_item_conf->phy_op_handle_range_used = false;
		list_item->phy_op_handle = DECT_PHY_RF_TOOL_RX_CONT_HANDLE;
		list_item_conf->rx.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS;
	}

	list_item_conf->rx.network_id = current_settings->common.network_id;

	/* No filters */
	list_item_conf->rx.filter.is_short_network_id_used = false;
	list_item_conf->rx.filter.short_network_id = 0;
	list_item_conf->rx.filter.receiver_identity = 0;

	if (!dect_phy_api_scheduler_list_item_add(list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add for RX failed", (__func__));
		ret = -EBUSY;
		dect_phy_api_scheduler_list_item_dealloc(list_item);
	}
	return ret;
}

static int dect_phy_rf_tool_tx_op_schedule(uint64_t start_time, uint32_t interval_mdm_ticks)
{
	struct dect_phy_api_scheduler_list_item_config *list_item_conf;
	struct dect_phy_api_scheduler_list_item *list_item =
		dect_phy_api_scheduler_list_item_alloc_tx_element(&list_item_conf);

	if (!list_item) {
		desh_error("Failed to allocate list item for TX operation");
		return -ENOMEM;
	}
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	union nrf_modem_dect_phy_hdr phy_header;

	list_item_conf->address_info.network_id = current_settings->common.network_id;
	list_item_conf->address_info.transmitter_long_rd_id =
		current_settings->common.transmitter_id;
	list_item_conf->address_info.receiver_long_rd_id = cmd_params->destination_transmitter_id;

	list_item_conf->cb_op_to_mdm = dect_phy_rf_tool_tx_to_mdm_cb;
	list_item_conf->cb_op_completed = NULL;
	list_item_conf->cb_op_completed_with_count = dect_phy_rf_tool_all_intervals_done;
	list_item_conf->interval_count_left = cmd_params->frame_repeat_count;
	list_item_conf->interval_mdm_ticks = interval_mdm_ticks;

	list_item_conf->channel = cmd_params->channel;
	list_item_conf->frame_time = start_time;
	list_item_conf->subslot_used = true;
	list_item_conf->start_slot = 0;
	list_item_conf->start_subslot = cmd_params->tx_frame_start_offset;

	list_item_conf->length_slots = 0;
	list_item_conf->length_subslots = cmd_params->tx_subslot_count;

	list_item_conf->tx.phy_lbt_period = 0;

	list_item->priority = DECT_PRIORITY1_TX;

	list_item_conf->tx.encoded_payload_pdu_size = rf_tool_data.scheduler_tx_data.tx_data_len;
	memcpy(list_item_conf->tx.encoded_payload_pdu, rf_tool_data.scheduler_tx_data.tx_data,
	       list_item_conf->tx.encoded_payload_pdu_size);

	list_item_conf->tx.header_type = DECT_PHY_HEADER_TYPE2;
	memcpy(&list_item_conf->tx.phy_header.type_2, &rf_tool_data.scheduler_tx_data.tx_phy_header,
	       sizeof(phy_header.type_2));

	list_item_conf->phy_op_handle_range_used = true;
	list_item->phy_op_handle = DECT_PHY_RF_TOOL_TX_HANDLE_START;
	list_item_conf->phy_op_handle_range_start = DECT_PHY_RF_TOOL_TX_HANDLE_START;
	list_item_conf->phy_op_handle_range_end = DECT_PHY_RF_TOOL_TX_HANDLE_END;

	if (!dect_phy_api_scheduler_list_item_add(list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed\n", (__func__));
		dect_phy_api_scheduler_list_item_dealloc(list_item);
		return -EBUSY;
	}

	return 0;
}

static int dect_phy_rf_tool_operations_schedule(uint64_t init_frame_start_time)
{
	struct dect_phy_rf_tool_params *cmd_params = &(rf_tool_data.cmd_params);
	uint64_t frame_start_time = init_frame_start_time;
	uint64_t time_now = dect_app_modem_time_now();
	uint32_t frame_len_mdm_ticks = rf_tool_data.frame_len_mdmticks;
	int ret = 0;

	if (init_frame_start_time == 0) {
		/* Initial start */
		frame_start_time =
			time_now + MS_TO_MODEM_TICKS(200);
	} else {
		frame_start_time = init_frame_start_time;
	}

	if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_TX) {
		ret = dect_phy_rf_tool_tx_op_schedule(frame_start_time, frame_len_mdm_ticks);
		if (ret) {
			desh_error("Failed to schedule TX operation on \"tx\" only mode: err %d",
				   ret);
			goto exit;
		}
		/* Store initial value for this  */
		if (init_frame_start_time == 0) {
			rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks =
				frame_start_time;
		}
	} else if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX) {
		ret = dect_phy_rf_tool_rx_op_schedule(frame_start_time, frame_len_mdm_ticks);
		if (ret) {
			desh_error("Failed to schedule RX operation on \"rx\" only mode: err %d",
				   ret);
			goto exit;
		}
		uint64_t offset = cmd_params->rx_frame_start_offset *
				  DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;

		/* Store initial value for this */
		if (init_frame_start_time == 0) {
			rf_tool_data.rx_metrics.last_pcc_stf_time = frame_start_time + offset;
		}
	} else if (cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		ret = dect_phy_rf_tool_rx_op_schedule(frame_start_time, frame_len_mdm_ticks);
		if (ret) {
			desh_error("Failed to schedule RX operation on \"rx_tx\" mode: err %d",
				   ret);
			goto exit;
		}
		uint64_t offset = cmd_params->rx_frame_start_offset *
				  DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;

		/* Store initial value for this */
		if (init_frame_start_time == 0) {
			rf_tool_data.rx_metrics.last_pcc_stf_time = frame_start_time + offset;
		}

		ret = dect_phy_rf_tool_tx_op_schedule(frame_start_time, frame_len_mdm_ticks);
		if (ret) {
			desh_error("Failed to schedule TX operation on \"rx_tx\" mode: err %d",
				   ret);
			goto exit;
		}

		/* Store initial value for this */
		if (init_frame_start_time == 0) {
			rf_tool_data.tx_metrics.tx_last_tx_scheduled_frame_time_mdm_ticks =
				frame_start_time;
		}
	} else {
		__ASSERT_NO_MSG(cmd_params->mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);

		ret = dect_phy_rf_tool_rx_op_schedule(frame_start_time, 0);
		if (ret) {
			desh_error("Failed to schedule RX operation on \"rx_cont\" mode: err %d",
				   ret);
			goto exit;
		}
	}
exit:
	return ret;
}

static int dect_phy_rf_tool_start(struct dect_phy_rf_tool_params *params, bool restart)
{
	int ret = 0;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	if (!restart) {
		memset(&rf_tool_data, 0, sizeof(struct dect_phy_rf_tool_tool_data));
	}
	rf_tool_data.frame_count_for_synch = 0;
	memset(&rf_tool_data.tx_metrics, 0, sizeof(rf_tool_data.tx_metrics));
	memset(&rf_tool_data.rx_metrics, 0, sizeof(rf_tool_data.rx_metrics));
	rf_tool_data.rx_metrics.rx_rssi_high_level = -127;
	rf_tool_data.rx_metrics.rx_rssi_low_level = 1;
	rf_tool_data.rx_metrics.rx_phy_transmit_pwr_high = 0;
	rf_tool_data.rx_metrics.rx_phy_transmit_pwr_low = 15;
	rf_tool_data.rx_metrics.rx_snr_low = 9999;
	rf_tool_data.rx_metrics.rx_snr_high = -9999;

	rf_tool_data.on_going = true;
	rf_tool_data.cmd_params = *params;

	/* Calculate frame len: */
	rf_tool_data.frame_len_subslots = dect_phy_rf_tool_frame_len_subslots_get(params->mode);
	rf_tool_data.frame_len_mdmticks = dect_phy_rf_tool_frame_len_mdmticks_get(params->mode);

	/* Fill TX data already in place now */
	if (!restart && (params->mode == DECT_PHY_RF_TOOL_MODE_TX ||
			 params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX)) {
		struct dect_phy_rf_tool_scheduler_tx_data *scheduler_tx_data =
			&(rf_tool_data.scheduler_tx_data);
		struct dect_phy_header_type2_format1_t *tx_header =
			(void *)&(scheduler_tx_data->tx_phy_header);
		uint16_t len_subslots = params->tx_subslot_count - 1;
		/* TX max amount of bytes that fills into given subslot amount: */
		uint16_t len_bytes =
			dect_common_utils_subslots_in_bytes(len_subslots, params->tx_mcs);

		if (len_bytes <= 0) {
			desh_error("%s: Unsupported slot/mcs combination", __func__);
			return -1;
		}
		if (len_bytes > DECT_DATA_MAX_LEN) {
			desh_error("%s: Too long TX data: %d bytes", __func__, len_bytes);
			return -1;
		}

		dect_common_utils_fill_with_repeating_pattern(scheduler_tx_data->tx_data,
							      len_bytes);
		scheduler_tx_data->tx_data_len = len_bytes;

		memset(tx_header, 0, sizeof(struct dect_phy_header_type2_format1_t));
		tx_header->packet_length = len_subslots;
		tx_header->packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SUBSLOTS;
		tx_header->format = DECT_PHY_HEADER_FORMAT_001; /* No HARQ feedback */
		tx_header->short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF);
		tx_header->transmitter_identity_hi =
			(uint8_t)(current_settings->common.transmitter_id >> 8);
		tx_header->transmitter_identity_lo =
			(uint8_t)(params->destination_transmitter_id & 0xFF);
		tx_header->df_mcs = params->tx_mcs;
		tx_header->transmit_power =
			dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm);
		tx_header->receiver_identity_hi =
			(uint8_t)(params->destination_transmitter_id >> 8);
		tx_header->receiver_identity_lo =
			(uint8_t)(params->destination_transmitter_id & 0xFF);
	}

	if (params->find_rx_sync && dect_phy_rf_tool_rx_mode(params->mode)) {
		/* RX until frame sync found: we get the synch from the 1st received decent PCC */
		struct nrf_modem_dect_phy_rx_params rx_op = {
			.handle = DECT_PHY_RF_TOOL_SYNCH_WAIT_RX_HANDLE,
			.start_time = 0,
			.mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
			.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
			.carrier = params->channel,
			.network_id = current_settings->common.network_id,
			.rssi_level = params->expected_rx_rssi_level,
			.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
		};

		rx_op.filter.is_short_network_id_used = true;
		rx_op.filter.short_network_id = current_settings->common.network_id;
		rx_op.filter.receiver_identity = current_settings->common.transmitter_id;
		rx_op.duration = UINT32_MAX;

		ret = dect_phy_common_rx_op(&rx_op);
		if (ret) {
			desh_error("(%s): nrf_modem_dect_phy_rx failed: ret %d, handle", (__func__),
				   ret, rx_op.handle);
			rf_tool_data.rx_metrics.rx_op_to_mdm_failure_count++;
		} else {
			rf_tool_data.rx_metrics.rx_wait_for_synch = true;
			rf_tool_data.rx_metrics.rx_op_to_mdm_ok_count++;
		}
	} else {
		/* Schedule operations right away if possible in active radio mode  */
		uint64_t next_possible_frame_time = 0;

		if (restart) {
			next_possible_frame_time = dect_phy_rf_tool_next_frame_time_get(restart);
		} else {
			enum nrf_modem_dect_phy_radio_mode radio_mode;

			ret = dect_phy_ctrl_current_radio_mode_get(&radio_mode);
			if (!ret && radio_mode != NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY) {
				uint64_t time_now = dect_app_modem_time_now();

				next_possible_frame_time =
					time_now + (3 * US_TO_MODEM_TICKS(
						current_settings->scheduler.scheduling_delay_us));
			}
		}
		ret = dect_phy_rf_tool_operations_schedule(next_possible_frame_time);
	}
	return ret;
}

static int dect_phy_rf_tool_frame_len_subslots_get(enum dect_phy_rf_tool_mode mode)
{
	int frame_len_subslots = 0;
	struct dect_phy_rf_tool_params *params = &(rf_tool_data.cmd_params);

	if (mode == DECT_PHY_RF_TOOL_MODE_RX) {
		frame_len_subslots = params->rx_frame_start_offset + params->rx_subslot_count +
				     params->rx_post_idle_subslot_count;
	} else if (mode == DECT_PHY_RF_TOOL_MODE_TX) {
		frame_len_subslots = params->tx_frame_start_offset + params->tx_subslot_count +
				     params->tx_post_idle_subslot_count;
	} else if (mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		frame_len_subslots = params->rx_frame_start_offset + params->rx_subslot_count +
				     params->rx_post_idle_subslot_count;

		if (params->tx_frame_start_offset > frame_len_subslots) {
			frame_len_subslots += (params->tx_frame_start_offset - frame_len_subslots);
		}
		frame_len_subslots +=
			(params->tx_subslot_count + params->tx_post_idle_subslot_count);
	} else {
		__ASSERT_NO_MSG(mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);

		if (params->peer_mode == DECT_PHY_RF_TOOL_MODE_TX) {
			frame_len_subslots = params->tx_frame_start_offset +
					     params->tx_subslot_count +
					     params->tx_post_idle_subslot_count;
		} else if (params->peer_mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
			frame_len_subslots = params->rx_frame_start_offset +
					     params->rx_subslot_count +
					     params->rx_post_idle_subslot_count;

			if (params->tx_frame_start_offset > frame_len_subslots) {
				frame_len_subslots +=
					(params->tx_frame_start_offset - frame_len_subslots);
			}
			frame_len_subslots +=
				(params->tx_subslot_count + params->tx_post_idle_subslot_count);
		} else {
			__ASSERT_NO_MSG(params->peer_mode == DECT_PHY_RF_TOOL_MODE_NONE);
			frame_len_subslots = 0;
		}
	}
	return frame_len_subslots;
}

static int dect_phy_rf_tool_frame_active_subslots_get(enum dect_phy_rf_tool_mode mode)
{
	int active_subslots = 0;
	struct dect_phy_rf_tool_params *params = &(rf_tool_data.cmd_params);

	if (mode == DECT_PHY_RF_TOOL_MODE_RX) {
		active_subslots = params->rx_subslot_count;
	} else if (mode == DECT_PHY_RF_TOOL_MODE_TX) {
		active_subslots = params->tx_subslot_count;
	} else if (mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
		active_subslots = params->rx_subslot_count + params->tx_subslot_count;
	} else {
		__ASSERT_NO_MSG(mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS);

		if (params->peer_mode == DECT_PHY_RF_TOOL_MODE_TX) {
			active_subslots = params->tx_subslot_count;
		} else if (params->peer_mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
			active_subslots = params->rx_subslot_count + params->tx_subslot_count;
		} else {
			__ASSERT_NO_MSG(params->peer_mode == DECT_PHY_RF_TOOL_MODE_NONE);
			active_subslots = 0;
		}
	}
	return active_subslots;
}

static uint64_t dect_phy_rf_tool_frame_len_mdmticks_get(enum dect_phy_rf_tool_mode mode)
{
	int frame_len_subslots = dect_phy_rf_tool_frame_len_subslots_get(mode);

	if (frame_len_subslots == 0) {
		return UINT32_MAX;
	} else {
		return frame_len_subslots * DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;
	}
}

static const char *dect_phy_rf_tool_test_mode_string_get(enum dect_phy_rf_tool_mode mode)
{
	switch (mode) {
	case DECT_PHY_RF_TOOL_MODE_RX:
		return "RX";
	case DECT_PHY_RF_TOOL_MODE_TX:
		return "TX";
	case DECT_PHY_RF_TOOL_MODE_RX_TX:
		return "RX_TX";
	case DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS:
		return "RX_CONT";
	default:
		return "UNKNOWN";
	}
}

static void dect_phy_rf_tool_start_print(void)
{
	struct dect_phy_rf_tool_params *params = &(rf_tool_data.cmd_params);

	desh_print("DECT PHY RF Tool started:");
	desh_print("  Mode:                        %s",
		   dect_phy_rf_tool_test_mode_string_get(params->mode));
	desh_print("  Channel:                     %d", params->channel);
	desh_print("  Continuous mode:             %s", params->continuous ? "true" : "false");
	if (params->mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS) {
		desh_print("  Find RX Sync:                %s",
			   params->find_rx_sync ? "true" : "false");
		desh_print("  Expected RX RSSI Level:      %d", params->expected_rx_rssi_level);
		if (params->peer_mode != DECT_PHY_RF_TOOL_MODE_NONE) {
			desh_print("  Peer mode:                   %s",
				   dect_phy_rf_tool_test_mode_string_get(params->peer_mode));
			desh_print("  Frame Repeat Count:          %d", params->frame_repeat_count);
			desh_print("  Frame length (in subslots):  %" PRIu64 "",
				   rf_tool_data.frame_len_subslots);
			desh_print("  Frame length (in mdm ticks): %" PRIu64 "",
				   rf_tool_data.frame_len_mdmticks);
			desh_print("  TX Start Time Subslots:      %d",
				   params->tx_frame_start_offset);
			desh_print("  TX Subslot Count:            %d", params->tx_subslot_count);
			desh_print("  TX Post Idle Subslot Count:  %d",
				   params->tx_post_idle_subslot_count);
		}
	} else {
		desh_print("  Destination Transmitter ID:  %d", params->destination_transmitter_id);
		desh_print("  Frame Repeat Count:          %d", params->frame_repeat_count);
		desh_print("  Frame length (in subslots):  %" PRIu64 "",
			   rf_tool_data.frame_len_subslots);
		desh_print("  Frame length (in mdm ticks): %" PRIu64 "",
			   rf_tool_data.frame_len_mdmticks);
		if (params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX ||
		    params->mode == DECT_PHY_RF_TOOL_MODE_RX) {
			desh_print("  RX Start Time Subslots:      %d",
				   params->rx_frame_start_offset);
			desh_print("  RX Subslot Count:            %d", params->rx_subslot_count);
			desh_print("  RX Post Idle Subslot Count:  %d",
				   params->rx_post_idle_subslot_count);
			desh_print("  Expected RX RSSI Level:      %d",
				   params->expected_rx_rssi_level);
			desh_print("  Find RX Sync:                %s",
				   params->find_rx_sync ? "true" : "false");
		}
		if (params->mode == DECT_PHY_RF_TOOL_MODE_TX ||
		    params->mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
			desh_print("  TX Power (dBm):              %d", params->tx_power_dbm);
			desh_print("  TX data length (in bytes):   %d",
				   rf_tool_data.scheduler_tx_data.tx_data_len);
			desh_print("  TX Start Time Subslots:      %d",
				   params->tx_frame_start_offset);
			desh_print("  TX Subslot Count:            %d", params->tx_subslot_count);
			desh_print("  TX Post Idle Subslot Count:  %d",
				   params->tx_post_idle_subslot_count);
		}
	}
}

int dect_phy_rf_tool_cmd_handle(struct dect_phy_rf_tool_params *params)
{
	int ret;

	if (rf_tool_data.on_going) {
		desh_error("rf_tool command already running");
		return -1;
	}

	dect_phy_rf_tool_phy_init();

	ret = dect_phy_rf_tool_start(params, false);
	if (ret) {
		rf_tool_data.on_going = false;
	} else {
		dect_phy_rf_tool_start_print();
	}
	return ret;
}

void dect_phy_rf_tool_cmd_stop(void)
{
	if (!rf_tool_data.on_going) {
		desh_print("No rf_tool command running - nothing to stop.");
		return;
	}

	dect_phy_rf_tool_msgq_non_data_op_add(DECT_PHY_RF_TOOL_EVT_CMD_DONE);
}

static int dect_phy_rf_tool_init(void)
{
	memset(&rf_tool_data, 0, sizeof(struct dect_phy_rf_tool_tool_data));

	return 0;
}

SYS_INIT(dect_phy_rf_tool_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
