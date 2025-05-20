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
#include "dect_common_settings.h"
#include "dect_common_utils.h"

#include "dect_phy_api_scheduler.h"
#include "dect_phy_ctrl.h"

#include "dect_phy_ping_pdu.h"
#include "dect_phy_ping.h"

K_SEM_DEFINE(ping_phy_api_init, 0, 1);

/**************************************************************************************************/

struct dect_phy_ping_rssi_scan_data {
	uint16_t current_scan_count;
	uint16_t current_saturated_count;
	uint16_t current_not_measured_count;
	uint16_t current_meas_fail_count;

	int8_t rssi_high_level;
	int8_t rssi_low_level;
	uint16_t current_channel; /* Currently scanned channel */
} curr_rssi_scan_data = {
	.current_meas_fail_count = 0,
	.current_not_measured_count = 0,
	.current_saturated_count = 0,
	.current_saturated_count = 0,
	.current_scan_count = 0,
	.rssi_high_level = -127,
	.rssi_low_level = 1,
};

struct dect_phy_ping_tx_metrics {
	uint32_t tx_total_data_amount;
	uint32_t tx_total_ping_req_count;
	uint32_t tx_total_ping_resp_count;
	uint32_t tx_failed_due_to_lbt;
};

struct dect_phy_ping_rx_metrics {
	uint32_t rx_total_ping_req_count;
	uint32_t rx_total_ping_resp_count;
	uint32_t rx_total_data_amount;
	uint32_t rx_pcc_crc_error_count;
	uint32_t rx_pdc_crc_error_count;
	uint32_t rx_out_of_seq_count;
	uint32_t rx_decode_error;

	int8_t rx_rssi_high_level;
	int8_t rx_rssi_low_level;
	int8_t rx_latest_rssi_level;
	int8_t rx_pdu_expected_rssi;
	uint8_t rx_last_pcc_mcs;
	uint8_t rx_phy_transmit_pwr;
	uint8_t rx_phy_transmit_pwr_low;
	uint8_t rx_phy_transmit_pwr_high;
	uint16_t rx_last_tx_id_from_pcc;

	int16_t rx_snr_low;
	int16_t rx_snr_high;

	int64_t rx_last_pcc_error_print_zticks;

	struct dect_phy_ping_rssi_scan_data rssi_scan_data_for_print;
};

#define DECT_PHY_PING_HARQ_TX_PROCESS_COUNT   2
#define DECT_PHY_PING_HARQ_TX_PROCESS_MAX_NBR 2

struct dect_phy_ping_harq_tx_res_data {
	uint32_t this_transmitter_id;
	uint32_t peer_transmitter_id;
};

struct dect_phy_ping_harq_tx_process_info {
	bool process_in_use;
	bool usable;
	bool new_data_ind_toggle;
	uint8_t last_redundancy_version;

	bool rtx_ongoing;

	uint8_t process_nbr;
	struct dect_phy_ping_harq_tx_res_data res_data;
	struct dect_phy_api_scheduler_list_item sche_list_item;
};

struct dect_phy_ping_client_data {
	int64_t tx_ping_req_tx_scheduled_mdm_ticks;

	bool tx_results_from_server_requested;
	bool tx_scheduler_intervals_done;
	bool tx_ping_resp_received;
	int64_t tx_last_scheduled_mdm_op_start_time_mdm_ticks;
	uint16_t tx_next_seq_nbr;
	uint16_t tx_data_len;
	uint8_t tx_data[DECT_DATA_MAX_LEN]; /* max data size in bytes when MCS4 + 4 slots */

	union nrf_modem_dect_phy_hdr tx_phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
	struct nrf_modem_dect_phy_rx_params rx_op; /* For ping resp */
};

struct dect_phy_ping_server_data {
	int64_t rx_enabled_z_ticks;
	struct nrf_modem_dect_phy_rx_params server_rx_op;
	uint16_t on_going_rx_count;

	int64_t rx_1st_data_received;
	int64_t rx_last_data_received;
	int64_t rx_last_pcc_received;
	uint16_t rx_last_seq_nbr;
	uint16_t rx_last_tx_id;
};

static struct dect_phy_ping_data {
	/* Common */
	bool on_going;
	int64_t time_started_ms;
	struct dect_phy_ping_params cmd_params;
	uint32_t restarted_count;

	struct dect_phy_ping_tx_metrics tx_metrics;
	struct dect_phy_ping_rx_metrics rx_metrics;

	/* For sending HARQ feedback for RX data when requested by the peer in a header */
	struct dect_phy_common_harq_feedback_data harq_feedback_data;

	/* Server side */
	struct dect_phy_ping_server_data server_data;

	/* Client side TX */
	struct dect_phy_ping_client_data client_data;
} ping_data;

/* HARQ process data */
static struct dect_phy_ping_harq_tx_process_info
	harq_processes[DECT_PHY_PING_HARQ_TX_PROCESS_COUNT];

/**************************************************************************************************/

static void dect_phy_ping_rx_on_pcc_crc_failure(void);
static void dect_phy_ping_rx_on_pdc_crc_failure(void);
static int dect_phy_ping_msgq_non_data_op_add(uint16_t event_id);
static int dect_phy_ping_rx_pdc_data_handle(struct dect_phy_data_rcv_common_params *params);

/**************************************************************************************************/

#define DECT_PHY_PING_EVENT_MDM_OP_COMPLETED		  1
#define DECT_PHY_PING_EVENT_SERVER_REPORT		  2
#define DECT_PHY_PING_EVENT_RX_PDC_DATA			  3
#define DECT_PHY_PING_EVENT_RX_PCC_CRC_ERROR		  4
#define DECT_PHY_PING_EVENT_RX_PDC_CRC_ERROR		  5
#define DECT_PHY_PING_EVENT_RX_PCC			  6
#define DECT_PHY_PING_EVENT_CMD_DONE			  7
#define DECT_PHY_PING_EVENT_CLIENT_SCHEDULER_PINGING_DONE 8
#define DECT_PHY_PING_EVENT_RSSI_COUNT_DONE		  9
#define DECT_PHY_PING_EVENT_HARQ_PAYLOAD_STORED		  10

static int dect_phy_ping_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size);

static void dect_phy_ping_data_reset(void)
{
	memset(&ping_data, 0, sizeof(struct dect_phy_ping_data));
}

static void dect_phy_ping_rx_metrics_reset(struct dect_phy_ping_params *params)
{
	memset(&ping_data.rx_metrics, 0, sizeof(struct dect_phy_ping_rx_metrics));

	ping_data.rx_metrics.rx_rssi_high_level = -127;
	ping_data.rx_metrics.rx_rssi_low_level = 1;
	ping_data.rx_metrics.rx_pdu_expected_rssi = -60;
	ping_data.rx_metrics.rx_latest_rssi_level = -60;
	ping_data.rx_metrics.rx_phy_transmit_pwr =
		dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm);
	ping_data.rx_metrics.rx_phy_transmit_pwr_low = 15; /* Bin: 1111 => 23dBm */
	ping_data.rx_metrics.rx_phy_transmit_pwr_high = 0; /* Bin: 0000 => -40dBm */
	ping_data.rx_metrics.rx_snr_low = 9999;
	ping_data.rx_metrics.rx_snr_high = -9999;
}

/**************************************************************************************************/

static bool dect_phy_ping_harq_process_next_new_data_ind_get(enum dect_harq_user harq_user)
{
	bool ret = !harq_processes[harq_user].new_data_ind_toggle;

	harq_processes[harq_user].new_data_ind_toggle = ret;
	return ret;
}

static void dect_phy_ping_harq_tx_process_table_init(uint8_t tx_harq_process_max_nbr)
{
	int i;

	for (i = 0; i < DECT_PHY_PING_HARQ_TX_PROCESS_COUNT; i++) {
		harq_processes[i].process_nbr = i;
		harq_processes[i].new_data_ind_toggle = false;
		harq_processes[i].usable = false;
		harq_processes[i].last_redundancy_version = 0;

		if (i < tx_harq_process_max_nbr) {
			harq_processes[i].usable = true;
		}
	}
}

static void dect_phy_ping_harq_feedback_tx_data_prefill(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);

	struct nrf_modem_dect_phy_tx_params harq_tx_params = {
		.start_time = 0,
		.handle = DECT_HARQ_FEEDBACK_TX_HANDLE,
		.carrier = cmd_params->channel,
		.network_id = current_settings->common.network_id,
		.phy_type = DECT_PHY_HEADER_TYPE2,
		.lbt_period = 0,
		.data_size = 4,
		.bs_cqi = 1, /* MCS-0, no meaning in our case */
	};
	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 0, /* = 1 slot */
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001,
		.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF),
		.transmitter_identity_hi = (uint8_t)(current_settings->common.transmitter_id >> 8),
		.transmitter_identity_lo =
			(uint8_t)(current_settings->common.transmitter_id & 0xFF),
	};
	dect_phy_ping_pdu_t tx_data_pdu = {
		.header.message_type = DECT_MAC_MESSAGE_TYPE_PING_HARQ_FEEDBACK,
		.header.transmitter_id = current_settings->common.transmitter_id,
	};

	dect_phy_ping_pdu_encode(ping_data.harq_feedback_data.encoded_data_to_send, &tx_data_pdu);

	harq_tx_params.data = ping_data.harq_feedback_data.encoded_data_to_send;
	ping_data.harq_feedback_data.params = harq_tx_params;
	ping_data.harq_feedback_data.header = header;
}

static bool dect_phy_ping_harq_process_init(enum dect_phy_common_role role,
					    struct dect_phy_ping_harq_tx_res_data res_data)
{
	enum dect_harq_user harq_user;

	if (role == DECT_PHY_COMMON_ROLE_CLIENT) {
		harq_user = DECT_HARQ_CLIENT;
	} else {
		__ASSERT_NO_MSG(role == DECT_PHY_COMMON_ROLE_SERVER);
		harq_user = DECT_HARQ_BEACON;
	}

	harq_processes[harq_user].process_in_use = false;
	harq_processes[harq_user].last_redundancy_version = 0;
	harq_processes[harq_user].rtx_ongoing = 0;
	harq_processes[harq_user].res_data.this_transmitter_id = res_data.this_transmitter_id;
	harq_processes[harq_user].res_data.peer_transmitter_id = res_data.peer_transmitter_id;
	dect_phy_ping_harq_feedback_tx_data_prefill();

	return true;
}

void dect_phy_ping_harq_process_round_start(enum dect_harq_user harq_user)
{
	if (harq_processes[harq_user].process_in_use) {
		desh_warn("Previous HARQ still on going and starting new round "
			  "- no response to prev data?");
	}
	harq_processes[harq_user].process_in_use = true;
	harq_processes[harq_user].last_redundancy_version = 0;
}

void dect_phy_ping_harq_process_round_end(enum dect_harq_user harq_user)
{
	harq_processes[harq_user].process_in_use = false;
	harq_processes[harq_user].last_redundancy_version = 0;
}

void dect_phy_ping_harq_store_tx_payload_data_scheduler_cb(
	struct dect_harq_tx_payload_data *harq_data)
{
	/* New ping tx is starting, store payload for possible re-tx */
	struct dect_phy_api_scheduler_list_item *in_sche_list_item = harq_data->sche_list_item;
	struct dect_phy_api_scheduler_list_item *ctrl_sche_list_item =
		&(harq_processes[harq_data->harq_user].sche_list_item);
	enum dect_harq_user harq_user = harq_data->harq_user;

	__ASSERT_NO_MSG(harq_user == DECT_HARQ_CLIENT); /* Only DECT_HARQ_CLIENT supported */
	__ASSERT_NO_MSG(in_sche_list_item->sched_config.tx.encoded_payload_pdu_size <=
			DECT_DATA_MAX_LEN);

	dect_phy_api_scheduler_list_item_mem_copy(ctrl_sche_list_item, in_sche_list_item);
	dect_phy_api_scheduler_list_item_dealloc(in_sche_list_item);

	dect_phy_ping_msgq_data_op_add(DECT_PHY_PING_EVENT_HARQ_PAYLOAD_STORED, (void *)&harq_user,
				       sizeof(enum dect_harq_user));
}

void dect_phy_ping_harq_client_nak_handle(void)
{
	uint64_t first_possible_tx;
	int8_t redundancy_version = dect_common_utils_harq_tx_next_redundancy_version_get(
		harq_processes[DECT_HARQ_CLIENT].last_redundancy_version);
	struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);

	if (redundancy_version < 0) {
		desh_warn("PCC: DECT_HARQ_CLIENT: all HARQ retransmissions done - no success.");
		dect_phy_ping_harq_process_round_end(DECT_HARQ_CLIENT);
		return;
	}
	uint64_t time_now = dect_app_modem_time_now();
	uint64_t elapsed_time = time_now - ping_data.client_data.tx_ping_req_tx_scheduled_mdm_ticks;
	int64_t remaining_time = MS_TO_MODEM_TICKS(cmd_params->timeout_msecs) - elapsed_time;

	if (remaining_time <= 0) {
		desh_warn("PCC: DECT_HARQ_CLIENT: no time to do a retransmit per NACK.");
		dect_phy_ping_harq_process_round_end(DECT_HARQ_CLIENT);
		return;
	}
	/* Latest ping request has been stored, modify that as new rtx item  */
	struct dect_phy_api_scheduler_list_item *sched_list_item =
		&(harq_processes[DECT_HARQ_CLIENT].sche_list_item);
	struct dect_phy_api_scheduler_list_item *new_sched_list_item =
		dect_phy_api_scheduler_list_item_create_new_copy(
			sched_list_item, sched_list_item->sched_config.frame_time, true);

	if (!new_sched_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_create_new_copy failed: No "
			   "memory to HARQ reTX");
		return;
	}

	struct dect_phy_api_scheduler_list_item_config *new_sched_list_item_conf =
		&(new_sched_list_item->sched_config);
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	first_possible_tx =
		time_now + US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us);

	/* Overwrite handle, complete_cb, tx/rx timing and redundancy version to header */
	new_sched_list_item->phy_op_handle = DECT_PHY_PING_CLIENT_RE_TX_HANDLE;
	new_sched_list_item_conf->cb_op_completed = NULL;
	new_sched_list_item_conf->frame_time = first_possible_tx;
	new_sched_list_item_conf->tx.combined_rx_op.duration =
		MAX(remaining_time, MS_TO_MODEM_TICKS(100));

	/* Only one timer and we don't want to have separate callback for completion */
	new_sched_list_item_conf->interval_mdm_ticks = 0;
	new_sched_list_item_conf->interval_count_left = 0;
	new_sched_list_item_conf->cb_op_to_mdm_with_interval_count_completed = NULL;
	new_sched_list_item_conf->cb_op_completed = NULL;

	struct dect_phy_header_type2_format0_t *header =
		(void *)&(new_sched_list_item_conf->tx.phy_header);

	header->df_redundancy_version = redundancy_version;
	header->transmit_power = dect_phy_ctrl_utils_mdm_next_supported_phy_tx_power_get(
		header->transmit_power, new_sched_list_item_conf->channel);

	/* Add tx operation to scheduler list */
	if (!dect_phy_api_scheduler_list_item_add(new_sched_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed", (__func__));
		dect_phy_api_scheduler_list_item_dealloc(new_sched_list_item);
	} else {
		harq_processes[DECT_HARQ_CLIENT].rtx_ongoing = true;
	}
}

/**************************************************************************************************/

static void dect_phy_ping_server_data_on_going_rx_count_decrease(void)
{
	if (ping_data.server_data.on_going_rx_count > 0) {
		ping_data.server_data.on_going_rx_count--;
	}
}

static bool dect_phy_ping_handle_is(uint32_t phy_op_handle)
{
	if (phy_op_handle == DECT_PHY_PING_CLIENT_TX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_CLIENT_RE_TX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_CLIENT_RX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_SERVER_RX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_SERVER_TX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_RESULTS_REQ_TX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_RESULTS_RESP_TX_HANDLE ||
	    phy_op_handle == DECT_PHY_PING_RESULTS_RESP_RX_HANDLE) {
		return true;
	} else {
		return false;
	}
}

static int dect_phy_ping_server_rx_start(uint64_t start_time)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	struct dect_phy_ping_params *params = &(ping_data.cmd_params);
	struct nrf_modem_dect_phy_rx_params rx_op = {
		.handle = DECT_PHY_PING_SERVER_RX_HANDLE,
		.start_time = start_time,
		.mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
		.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
		.carrier = params->channel,
		.network_id = current_settings->common.network_id,
		.rssi_level = params->expected_rx_rssi_level,
	};
	int ret;

	rx_op.filter.is_short_network_id_used = true;
	rx_op.filter.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF);
	rx_op.filter.receiver_identity = current_settings->common.transmitter_id;
	rx_op.duration = UINT32_MAX;

	if (ping_data.cmd_params.rssi_reporting_enabled) {
		rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS;
	} else {
		rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF;
	}

	ping_data.server_data.server_rx_op = rx_op;

	ret = dect_phy_common_rx_op(&rx_op);
	if (ret) {
		printk("nrf_modem_dect_phy_rx failed: ret %d, handle %d\n", ret, rx_op.handle);
		ping_data.on_going = false;
	} else {
		ping_data.server_data.rx_enabled_z_ticks = k_uptime_get();
		;
		ping_data.server_data.on_going_rx_count++;
	}
	return ret;
}

/**************************************************************************************************/

static uint16_t dect_phy_ping_auto_pwr_ctrl_target_pwr_calculate(void)
{
	int16_t target_rssi = ping_data.rx_metrics.rx_pdu_expected_rssi;
	int16_t target_power =
		dect_common_utils_phy_tx_power_to_dbm(ping_data.rx_metrics.rx_phy_transmit_pwr);
	uint16_t rssi_diff = 0;

	if (ping_data.rx_metrics.rx_latest_rssi_level > target_rssi) {
		/* Decrease power compared to received power */
		rssi_diff = abs(ping_data.rx_metrics.rx_latest_rssi_level - target_rssi);
		target_power = target_power - rssi_diff;
	} else {
		/* Increase/keep the power compared to received power */
		rssi_diff = abs(target_rssi - ping_data.rx_metrics.rx_latest_rssi_level);
		target_power = target_power + rssi_diff;
	}
	/* Check that not over the permitted max on used channel */
	int8_t max_permitted_pwr_dbm = dect_phy_ctrl_utils_mdm_max_tx_pwr_dbm_get_by_channel(
		ping_data.cmd_params.channel);

	if (target_power > max_permitted_pwr_dbm) {
		target_power = max_permitted_pwr_dbm;
	}

	return target_power;
}

/**************************************************************************************************/

static void dect_phy_ping_client_data_tx_total_data_amount_decrease(void)
{
	if (ping_data.tx_metrics.tx_total_data_amount >= ping_data.client_data.tx_op.data_size) {
		/* TX was not done */
		ping_data.tx_metrics.tx_total_data_amount -= ping_data.client_data.tx_op.data_size;
	}
}

/**************************************************************************************************/

static void dect_phy_ping_mdm_op_complete_cb(
	const struct nrf_modem_dect_phy_op_complete_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_completed_params ping_op_completed_params = {
		.handle = evt->handle,
		.temperature = evt->temp,
		.status = evt->err,
		.time = *time,
	};

	dect_phy_api_scheduler_mdm_op_completed(&ping_op_completed_params);
	dect_phy_ping_msgq_data_op_add(DECT_PHY_PING_EVENT_MDM_OP_COMPLETED,
				       (void *)&ping_op_completed_params,
				       sizeof(struct dect_phy_common_op_completed_params));
}

static void dect_phy_ping_mdm_pcc_cb(
	const struct nrf_modem_dect_phy_pcc_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pcc_rcv_params ctrl_pcc_op_params;
	struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);
	uint64_t harq_feedback_start_time = 0;

	ctrl_pcc_op_params.pcc_status = *evt;
	ctrl_pcc_op_params.phy_header = evt->hdr;
	ctrl_pcc_op_params.time = *time;
	ctrl_pcc_op_params.stf_start_time = evt->stf_start_time;
	/* Others from struct dect_phy_common_op_pcc_rcv_params are not needed */

	/* Provide HARQ feedback if requested */
	if (evt->header_status == NRF_MODEM_DECT_PHY_HDR_STATUS_VALID &&
	    evt->phy_type == DECT_PHY_HEADER_TYPE2) {
		struct dect_phy_header_type2_format0_t *header = (void *)&evt->hdr;

		if (cmd_params->role == DECT_PHY_COMMON_ROLE_SERVER &&
		    header->format == DECT_PHY_HEADER_FORMAT_000 &&
		    header->short_network_id ==
			    ping_data.harq_feedback_data.header.short_network_id &&
		    header->receiver_identity_hi ==
			    ping_data.harq_feedback_data.header.transmitter_identity_hi &&
		    header->receiver_identity_lo ==
			    ping_data.harq_feedback_data.header.transmitter_identity_lo) {
			struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

			struct nrf_modem_dect_phy_tx_params harq_tx =
				ping_data.harq_feedback_data.params;
			struct dect_phy_header_type2_format1_t feedback_header;
			uint16_t len_slots = header->packet_length + 1;

			/* HARQ feedback requested */
			union nrf_modem_dect_phy_hdr phy_header;

			feedback_header = ping_data.harq_feedback_data.header;
			feedback_header.format = DECT_PHY_HEADER_FORMAT_001;
			feedback_header.df_mcs = 0;
			feedback_header.transmit_power =
				dect_phy_ctrl_utils_mdm_next_supported_phy_tx_power_get(
					header->transmit_power, harq_tx.carrier);
			feedback_header.receiver_identity_hi = header->transmitter_identity_hi;
			feedback_header.receiver_identity_lo = header->transmitter_identity_lo;
			feedback_header.spatial_streams = header->spatial_streams;
			feedback_header.feedback.format1.format = 1;
			feedback_header.feedback.format1.CQI = 1;
			feedback_header.feedback.format1.harq_process_number0 =
				header->df_harq_process_number;

			/* ACK/NACK: According to CRC: */
			feedback_header.feedback.format1.transmission_feedback0 = 0;
			feedback_header.feedback.format1.buffer_status =
				0; /* No meaning in our case */
			memcpy(&phy_header.type_2, &feedback_header, sizeof(phy_header.type_2));
			harq_tx.phy_header = &phy_header;
			harq_feedback_start_time =
				evt->stf_start_time +
				(len_slots * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
				(current_settings->harq.harq_feedback_tx_delay_subslot_count *
				 DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);
			harq_tx.start_time = harq_feedback_start_time;

			int err = nrf_modem_dect_phy_tx_harq(&harq_tx);

			if (err) {
				printk("nrf_modem_dect_phy_tx_harq() failed: %d\n", err);
			}
		}
	}
	dect_phy_ping_msgq_data_op_add(DECT_PHY_PING_EVENT_RX_PCC, (void *)&ctrl_pcc_op_params,
				       sizeof(struct dect_phy_common_op_pcc_rcv_params));
}

static void dect_phy_ping_mdm_pcc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pcc_crc_failure_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pcc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_ping_msgq_data_op_add(DECT_PHY_PING_EVENT_RX_PCC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pcc_crc_fail_params));
}

static void dect_phy_ping_mdm_pdc_cb(const struct nrf_modem_dect_phy_pdc_event *evt,
				     uint64_t *time)
{
	int16_t rssi_level = evt->rssi_2 / 2;
	struct dect_phy_commmon_op_pdc_rcv_params ping_pdc_op_params = { 0 };

	ping_pdc_op_params.rx_status = *evt;

	ping_pdc_op_params.data_length = evt->len;
	ping_pdc_op_params.time = *time;

	ping_pdc_op_params.rx_pwr_dbm =
		dect_common_utils_phy_tx_power_to_dbm(ping_data.rx_metrics.rx_phy_transmit_pwr);
	ping_pdc_op_params.rx_mcs = ping_data.rx_metrics.rx_last_pcc_mcs;
	ping_pdc_op_params.rx_rssi_level_dbm = rssi_level;
	ping_pdc_op_params.rx_channel = ping_data.cmd_params.channel;

	/* Others from struct dect_phy_commmon_op_pdc_rcv_params are not needed */

	if (evt->len <= sizeof(ping_pdc_op_params.data)) {
		memcpy(ping_pdc_op_params.data, evt->data, evt->len);
		dect_phy_ping_msgq_data_op_add(DECT_PHY_PING_EVENT_RX_PDC_DATA,
					       (void *)&ping_pdc_op_params,
					       sizeof(struct dect_phy_commmon_op_pdc_rcv_params));
	} else {
		printk("Received data is too long to be received by PING TH - discarded (len %d, "
		       "buf size %d)\n",
		       evt->len, sizeof(ping_pdc_op_params.data));
	}
}

static void dect_phy_ping_mdm_pdc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt,
	uint64_t *time)
{
	struct dect_phy_common_op_pdc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_ping_msgq_data_op_add(DECT_PHY_PING_EVENT_RX_PDC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pdc_crc_fail_params));
}

/**************************************************************************************************/

static void dect_phy_ping_rssi_scan_print(void)
{
	struct dect_phy_ping_rssi_scan_data *rssi_scan_data =
		&(ping_data.rx_metrics.rssi_scan_data_for_print);

	if (ping_data.cmd_params.rssi_reporting_enabled && rssi_scan_data->current_scan_count) {
		struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
		char color[10];

		sprintf(color, "%s", ANSI_COLOR_GREEN);

		desh_print("Background RSSI scanning results:");
		desh_print("  channel                     %d", ping_data.cmd_params.channel);
		desh_print("  scan count                  %d", rssi_scan_data->current_scan_count);

		if (rssi_scan_data->rssi_low_level > current_settings->rssi_scan.busy_threshold) {
			sprintf(color, "%s", ANSI_COLOR_RED);
		} else if (rssi_scan_data->rssi_high_level <=
			   current_settings->rssi_scan.free_threshold) {
			sprintf(color, "%s", ANSI_COLOR_GREEN);
		} else {
			sprintf(color, "%s", ANSI_COLOR_YELLOW);
		}
		desh_print("%s  highest RSSI                 %d%s", color,
			   rssi_scan_data->rssi_high_level, ANSI_RESET_ALL);
		desh_print("  lowest RSSI                 %d", rssi_scan_data->rssi_low_level);
		if (rssi_scan_data->current_saturated_count) {
			desh_print("    saturations               %d",
				   rssi_scan_data->current_saturated_count);
		}
		if (rssi_scan_data->current_not_measured_count) {
			desh_print("    not measured              %d",
				   rssi_scan_data->current_not_measured_count);
		}
		if (rssi_scan_data->current_meas_fail_count) {
			desh_print("    measurement failed        %d",
				   rssi_scan_data->current_meas_fail_count);
		}
	}
}

static void dect_phy_ping_rssi_curr_data_reset(void)
{
	memset(&curr_rssi_scan_data, 0, sizeof(struct dect_phy_ping_rssi_scan_data));
	curr_rssi_scan_data.rssi_high_level = -127;
	curr_rssi_scan_data.rssi_low_level = 1;
}

static void dect_phy_ping_rssi_done_evt_send(void)
{
	ping_data.rx_metrics.rssi_scan_data_for_print = curr_rssi_scan_data;
	dect_phy_ping_rssi_curr_data_reset();

	dect_phy_ping_msgq_non_data_op_add(DECT_PHY_PING_EVENT_RSSI_COUNT_DONE);
}

static void dect_phy_ping_rssi_ntf_handle(enum nrf_modem_dect_phy_err status,
					  struct nrf_modem_dect_phy_rssi_event const *p_result)
{
	if (status == NRF_MODEM_DECT_PHY_SUCCESS) {
		__ASSERT_NO_MSG(p_result != NULL);

		/* Store response */
		for (int i = 0; i < p_result->meas_len; i++) {
			int8_t curr_meas = p_result->meas[i];

			if (curr_meas > 0) {
				curr_rssi_scan_data.current_saturated_count++;
			} else if (curr_meas == NRF_MODEM_DECT_PHY_RSSI_NOT_MEASURED) {
				curr_rssi_scan_data.current_not_measured_count++;
			} else {
				if (curr_meas > curr_rssi_scan_data.rssi_high_level) {
					curr_rssi_scan_data.rssi_high_level = curr_meas;
				}
				if (curr_meas < curr_rssi_scan_data.rssi_low_level) {
					curr_rssi_scan_data.rssi_low_level = curr_meas;
				}
			}
		}
	} else {
		curr_rssi_scan_data.current_meas_fail_count++;
	}
	curr_rssi_scan_data.current_scan_count++;

	if (curr_rssi_scan_data.current_scan_count == 200) {
		dect_phy_ping_rssi_done_evt_send();
	}
}

static void dect_phy_ping_mdm_rssi_cb(const struct nrf_modem_dect_phy_rssi_event *evt)
{
	dect_phy_ping_rssi_ntf_handle(NRF_MODEM_DECT_PHY_SUCCESS, evt);
}

/**************************************************************************************************/


static void dect_phy_ping_client_tx_all_intervals_done(uint32_t handle)
{
	__ASSERT_NO_MSG(handle == DECT_PHY_PING_CLIENT_TX_HANDLE);

	/* Pinging done in a scheduler. Request results from server. */
	dect_phy_ping_msgq_non_data_op_add(DECT_PHY_PING_EVENT_CLIENT_SCHEDULER_PINGING_DONE);
}

static void dect_phy_ping_client_tx_complete_cb(
	struct dect_phy_common_op_completed_params *params, uint64_t tx_frame_time)
{
	if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
		ping_data.client_data.tx_ping_req_tx_scheduled_mdm_ticks = tx_frame_time;
		ping_data.tx_metrics.tx_total_data_amount += ping_data.client_data.tx_data_len;
	}
}

static void dect_phy_ping_client_tx_schedule(uint64_t first_possible_tx)
{
	struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);

	struct dect_phy_api_scheduler_list_item_config *sched_list_item_conf;
	struct dect_phy_api_scheduler_list_item *sched_list_item =
		dect_phy_api_scheduler_list_item_alloc_tx_element(&sched_list_item_conf);
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	if (!sched_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_alloc_tx_element failed: No "
			   "memory to schedule a ping");
		goto exit;
	}
	sched_list_item_conf->address_info.network_id = current_settings->common.network_id;
	sched_list_item_conf->address_info.transmitter_long_rd_id =
		current_settings->common.transmitter_id;
	sched_list_item_conf->address_info.receiver_long_rd_id =
		cmd_params->destination_transmitter_id;

	sched_list_item_conf->channel = cmd_params->channel;
	sched_list_item_conf->frame_time = first_possible_tx;
	sched_list_item_conf->start_slot = 0;

	sched_list_item_conf->tx.cb_harq_tx_store = NULL;
	sched_list_item_conf->tx.harq_feedback_requested = false;

	/* Only with HARQ for possible retransmissions based on NACK */
	if (cmd_params->use_harq) {
		sched_list_item_conf->tx.cb_harq_tx_store =
			dect_phy_ping_harq_store_tx_payload_data_scheduler_cb;
		sched_list_item_conf->tx.harq_user = DECT_HARQ_CLIENT;
		sched_list_item_conf->tx.harq_feedback_requested = true;
	}

	sched_list_item_conf->cb_op_completed = dect_phy_ping_client_tx_complete_cb;

	sched_list_item_conf->interval_mdm_ticks =
		SECONDS_TO_MODEM_TICKS(cmd_params->interval_secs);
	sched_list_item_conf->interval_count_left = cmd_params->ping_count;
	sched_list_item_conf->cb_op_to_mdm_with_interval_count_completed =
		dect_phy_ping_client_tx_all_intervals_done;

	sched_list_item_conf->length_slots = cmd_params->slot_count;
	sched_list_item_conf->length_subslots = 0;

	sched_list_item_conf->tx.phy_lbt_period = ping_data.client_data.tx_op.lbt_period;
	sched_list_item_conf->tx.phy_lbt_rssi_threshold_max =
		ping_data.client_data.tx_op.lbt_rssi_threshold_max;

	sched_list_item->priority = DECT_PRIORITY1_TX;

	sched_list_item_conf->tx.combined_tx_rx_use = true;
	sched_list_item_conf->tx.combined_rx_op = ping_data.client_data.rx_op;

	sched_list_item->sched_config.tx.encoded_payload_pdu_size =
		ping_data.client_data.tx_op.data_size;
	memcpy(sched_list_item->sched_config.tx.encoded_payload_pdu,
	       ping_data.client_data.tx_op.data,
	       sched_list_item->sched_config.tx.encoded_payload_pdu_size);
	sched_list_item->sched_config.tx.header_type = DECT_PHY_HEADER_TYPE2;
	memcpy(&sched_list_item->sched_config.tx.phy_header.type_2,
	       &ping_data.client_data.tx_phy_header.type_2,
	       sizeof(ping_data.client_data.tx_phy_header.type_2));

	sched_list_item->phy_op_handle = ping_data.client_data.tx_op.handle;

	/* Add beacon tx operation to scheduler list */
	if (!dect_phy_api_scheduler_list_item_add(sched_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed\n", (__func__));
		dect_phy_api_scheduler_list_item_dealloc(sched_list_item);
		goto exit;
	}
	ping_data.tx_metrics.tx_total_data_amount +=
		sched_list_item->sched_config.tx.encoded_payload_pdu_size;
	ping_data.client_data.tx_last_scheduled_mdm_op_start_time_mdm_ticks =
		sched_list_item_conf->frame_time;
	ping_data.client_data.tx_ping_resp_received = false;

exit:
}

static void dect_phy_ping_cmd_done(void)
{
	if (!ping_data.on_going) {
		desh_error("%s called when not on going - caller %pS",
			(__func__),
			__builtin_return_address(0));
		return;
	}
	ping_data.on_going = false;

	/* Mdm phy api deinit is done by dect_phy_ctrl */

	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_PING_CMD_DONE);
}

static int dect_phy_ping_client_start(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_ping_params *params = &(ping_data.cmd_params);

	/* MAC spec, ch 6.2.1:
	 * The packet length is a signalled numerical value plus one subslot or slot.
	 * In practice in phy header: packet len with slot type: 0 in header means 1, 1 = 2, 7 = 8
	 * etc.
	 */
	struct dect_phy_header_type2_format0_t header = {
		.packet_length = params->slot_count - 1,
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001, /* No HARQ feedback */
		.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF),
		.transmitter_identity_hi = (uint8_t)(current_settings->common.transmitter_id >> 8),
		.transmitter_identity_lo =
			(uint8_t)(current_settings->common.transmitter_id & 0xFF),
		.df_mcs = params->tx_mcs,
		.transmit_power = dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm),
		.receiver_identity_hi = (uint8_t)(params->destination_transmitter_id >> 8),
		.receiver_identity_lo = (uint8_t)(params->destination_transmitter_id & 0xFF),
		.df_harq_process_number = DECT_HARQ_CLIENT,
		.df_new_data_indication_toggle = 1,
		.df_redundancy_version = 0, /* 1st tx */
		.spatial_streams = 2,
		.feedback = {
				.format1 = {
						.format = 0,
						.CQI = 1,
						.harq_process_number0 = DECT_HARQ_CLIENT,
						.transmission_feedback0 = 1,
						.buffer_status = 5, /* 0101 = 128 <= BS <= 256 */
					},
			},
	};
	dect_phy_ping_pdu_t ping_pdu;
	uint8_t *encoded_data_to_send = ping_data.client_data.tx_data;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx =
		time_now + (5 * US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));

	/* Sending max amount of data that can be encoded to given slot amount */
	int16_t ping_pdu_byte_count =
		dect_common_utils_slots_in_bytes(params->slot_count - 1, params->tx_mcs);

	if (ping_pdu_byte_count <= 0) {
		desh_error("Unsupported slot/mcs combination");
		return -1;
	}

	header.format = DECT_PHY_HEADER_FORMAT_001;

	if (params->use_harq) {
		header.df_new_data_indication_toggle =
			dect_phy_ping_harq_process_next_new_data_ind_get(DECT_HARQ_CLIENT);
		header.format = DECT_PHY_HEADER_FORMAT_000;
		header.feedback.format1.format = 1;
		header.feedback.format1.harq_process_number0 = DECT_HARQ_CLIENT;
	}

	desh_print("Starting ping client on channel %d:\n"
		   "  byte count per TX: %d, slots %d, interval %d secs,\n"
		   "  mcs %d, LBT period %d, count %d, timeout %d msecs, HARQ: %s,\n"
		   "  expected RSSI level on RX %d.",
		   params->channel, ping_pdu_byte_count, params->slot_count, params->interval_secs,
		   params->tx_mcs, params->tx_lbt_period_symbols, params->ping_count,
		   params->timeout_msecs, (params->use_harq) ? "yes" : "no",
		   params->expected_rx_rssi_level);

	uint16_t ping_pdu_payload_byte_count =
		ping_pdu_byte_count - DECT_PHY_PING_TX_DATA_PDU_LEN_WITHOUT_PAYLOAD;

	/* Encode ping pdu */
	memcpy(&ping_data.client_data.tx_phy_header.type_2, &header,
	       sizeof(ping_data.client_data.tx_phy_header.type_2));

	ping_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PING_REQUEST;
	ping_pdu.header.pwr_ctrl_expected_rssi_level_dbm =
		params->pwr_ctrl_pdu_expected_rx_rssi_level;
	ping_pdu.header.transmitter_id = current_settings->common.transmitter_id;
	ping_pdu.message.tx_data.seq_nbr = 1;
	ping_pdu.message.tx_data.payload_length = ping_pdu_payload_byte_count;
	dect_common_utils_fill_with_repeating_pattern(ping_pdu.message.tx_data.pdu_payload,
						      ping_pdu_payload_byte_count);
	dect_phy_ping_pdu_encode(encoded_data_to_send, &ping_pdu);

	ping_data.time_started_ms = k_uptime_get();

	memset(&ping_data.tx_metrics, 0, sizeof(struct dect_phy_ping_tx_metrics));

	/* Fill TX operation */
	ping_data.client_data.tx_next_seq_nbr = ping_pdu.message.tx_data.seq_nbr;
	ping_data.client_data.tx_data_len = ping_pdu_byte_count;

	ping_data.client_data.tx_op.handle = DECT_PHY_PING_CLIENT_TX_HANDLE;
	ping_data.client_data.tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	ping_data.client_data.tx_op.carrier = params->channel;
	ping_data.client_data.tx_op.data_size = ping_pdu_byte_count;
	ping_data.client_data.tx_op.data = encoded_data_to_send;
	ping_data.client_data.tx_op.lbt_period = params->tx_lbt_period_symbols *
		NRF_MODEM_DECT_SYMBOL_DURATION;
	ping_data.client_data.tx_op.lbt_rssi_threshold_max =
		params->tx_lbt_rssi_busy_threshold_dbm;

	ping_data.client_data.tx_op.network_id = current_settings->common.network_id;
	ping_data.client_data.tx_op.phy_header = &ping_data.client_data.tx_phy_header;
	ping_data.client_data.tx_op.phy_type = DECT_PHY_HEADER_TYPE2;

	/* For ping response: */
	ping_data.client_data.rx_op.handle = DECT_PHY_PING_CLIENT_RX_HANDLE;

	/* RX time is relative from TX end. We use HARQ setting here even if no HARQ */
	ping_data.client_data.rx_op.start_time =
		current_settings->harq.harq_feedback_rx_delay_subslot_count *
		DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;

	ping_data.client_data.rx_op.network_id = current_settings->common.network_id;
	ping_data.client_data.rx_op.filter.is_short_network_id_used = true;
	ping_data.client_data.rx_op.filter.short_network_id =
		(uint8_t)(current_settings->common.network_id & 0xFF);
	ping_data.client_data.rx_op.filter.receiver_identity =
		current_settings->common.transmitter_id;
	ping_data.client_data.rx_op.carrier = params->channel;

	ping_data.client_data.rx_op.duration = MS_TO_MODEM_TICKS(params->timeout_msecs);
	ping_data.client_data.rx_op.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS;
	ping_data.client_data.rx_op.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED;
	ping_data.client_data.rx_op.rssi_level = params->expected_rx_rssi_level;

	if (params->rssi_reporting_enabled) {
		ping_data.client_data.rx_op.rssi_interval =
			NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS;
	} else {
		ping_data.client_data.rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF;
	}

	dect_phy_ping_client_tx_schedule(first_possible_tx);

	return 0;
}

enum dect_phy_ping_server_restart_mode {
	START = 0,
	RESTART,
	RESTART_AFTER_RESULT_SENDING,
};

static int dect_phy_ping_start(struct dect_phy_ping_params *params,
			       enum dect_phy_ping_server_restart_mode restart_mode,
			       uint64_t start_time)
{
	int ret = 0;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	if (restart_mode == RESTART) {
		ping_data.restarted_count++;
	} else {
		dect_phy_ping_data_reset();

		ping_data.time_started_ms = k_uptime_get();
		dect_phy_ping_rx_metrics_reset(params);
		dect_phy_ping_harq_tx_process_table_init(DECT_PHY_PING_HARQ_TX_PROCESS_MAX_NBR);
	}

	ping_data.on_going = true;
	ping_data.cmd_params = *params;

	struct dect_phy_ping_harq_tx_res_data res_data = {
		.peer_transmitter_id = params->destination_transmitter_id,
		.this_transmitter_id = current_settings->common.transmitter_id,
	};

	/* Reserving HARQ process always */
	if (!dect_phy_ping_harq_process_init(params->role, res_data)) {
		desh_warn("Cannot reserve HARQ process. HARQ disabled");
	}

	if (params->role == DECT_PHY_COMMON_ROLE_CLIENT) {
		ret = dect_phy_ping_client_start();
	} else {
		__ASSERT_NO_MSG(params->role == DECT_PHY_COMMON_ROLE_SERVER);

		desh_print("Starting ping server RX: channel %d, exp_rssi_level %d.",
			   params->channel, params->expected_rx_rssi_level);
		ret = dect_phy_ping_server_rx_start(start_time);
	}
	return ret;
}

/**************************************************************************************************/

static int dect_phy_ping_tx_request_results(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_ping_params *params = &(ping_data.cmd_params);

	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 0,
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001,
		.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF),
		.transmitter_identity_hi = (uint8_t)(current_settings->common.transmitter_id >>
						     8),
		.transmitter_identity_lo =
			(uint8_t)(current_settings->common.transmitter_id & 0xFF),
		.df_mcs = 0,
		.transmit_power = dect_phy_ctrl_utils_mdm_next_supported_phy_tx_power_get(
			dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm),
			params->channel),
		.receiver_identity_hi = (uint8_t)(params->destination_transmitter_id >> 8),
		.receiver_identity_lo = (uint8_t)(params->destination_transmitter_id & 0xFF),
		.feedback.format1.format = 0,
	};
	union nrf_modem_dect_phy_hdr phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
	dect_phy_ping_pdu_t ping_pdu;
	uint8_t encoded_data_to_send[64];
	int ret = 0;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx = time_now + SECONDS_TO_MODEM_TICKS(2);

	if (params->pwr_ctrl_automatic) {
		header.transmit_power = dect_common_utils_dbm_to_phy_tx_power(
			dect_phy_ping_auto_pwr_ctrl_target_pwr_calculate());
	}

	memcpy(&phy_header.type_2, &header, sizeof(phy_header.type_2));

	ping_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ;
	ping_pdu.header.transmitter_id = current_settings->common.transmitter_id;
	ping_pdu.header.pwr_ctrl_expected_rssi_level_dbm =
		params->pwr_ctrl_pdu_expected_rx_rssi_level;
	ping_pdu.message.results_req.unused = 0;
	dect_phy_ping_pdu_encode(encoded_data_to_send, &ping_pdu);

	tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	tx_op.carrier = params->channel;
	tx_op.data_size = DECT_PHY_PING_RESULTS_REQ_LEN;
	tx_op.data = encoded_data_to_send;
	tx_op.lbt_period = 0;
	tx_op.lbt_rssi_threshold_max = current_settings->rssi_scan.busy_threshold;
	tx_op.network_id = current_settings->common.network_id;
	tx_op.phy_header = &phy_header;
	tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
	tx_op.handle = DECT_PHY_PING_RESULTS_REQ_TX_HANDLE;
	tx_op.start_time = first_possible_tx;

	ret = nrf_modem_dect_phy_tx(&tx_op);
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_tx failed %d (handle %d)\n", (__func__), ret,
			   tx_op.handle);
	} else {
		/* RX for the results response */
		struct nrf_modem_dect_phy_rx_params rx_op = {
			.handle = DECT_PHY_PING_RESULTS_RESP_RX_HANDLE,
			.start_time =
				first_possible_tx + (3 * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS),
			.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
			.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
			.carrier = params->channel,
			.network_id = current_settings->common.network_id,
			.rssi_level = params->expected_rx_rssi_level,
		};

		rx_op.filter.is_short_network_id_used = true;
		rx_op.filter.short_network_id =
			(uint8_t)(current_settings->common.network_id & 0xFF);
		rx_op.filter.receiver_identity = current_settings->common.transmitter_id;
		rx_op.duration = SECONDS_TO_MODEM_TICKS(5);

		if (ping_data.cmd_params.rssi_reporting_enabled) {
			rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_24_SLOTS;
		} else {
			rx_op.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF;
		}
		ret = dect_phy_common_rx_op(&rx_op);
		if (ret) {
			desh_error("nrf_modem_dect_phy_rx failed: ret %d, handle", ret,
				   rx_op.handle);
		}
	}

	return ret;
}

static int
dect_phy_ping_client_report_local_results_and_req_server_results(int64_t *elapsed_time_ms_ptr)
{
	if (ping_data.client_data.tx_results_from_server_requested) {
		return 0;
	}

	int64_t time_orig_started = ping_data.time_started_ms;
	int64_t elapsed_time_ms = k_uptime_delta(&time_orig_started);
	int ret = 0;

	if (elapsed_time_ms_ptr != NULL) {
		elapsed_time_ms = *elapsed_time_ms_ptr;
	}
	double elapsed_time_secs = elapsed_time_ms / 1000;

	/* Stop client ping tx and rx for ping_resp in a scheduler */
	dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle(
		DECT_PHY_PING_CLIENT_TX_HANDLE);
	dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle(
		DECT_PHY_PING_CLIENT_RX_HANDLE);

	desh_print("ping client operation completed:");
	desh_print("  elapsed time:                            %.2f seconds", elapsed_time_secs);
	desh_print("  tx: total amount of data sent:           %d bytes",
		   ping_data.tx_metrics.tx_total_data_amount);
	desh_print("  tx: ping req count:                      %d",
		   ping_data.tx_metrics.tx_total_ping_req_count);
	desh_print("  tx: ping failed due to LBT count:        %d",
		   ping_data.tx_metrics.tx_failed_due_to_lbt);
	desh_print("  rx: ping resp count:                     %d",
		   ping_data.rx_metrics.rx_total_ping_resp_count);
	desh_print("  rx: total amount of data:                %d bytes",
		   ping_data.rx_metrics.rx_total_data_amount);
	desh_print("  rx: out of sequence count:               %d",
		   ping_data.rx_metrics.rx_out_of_seq_count);
	desh_print("  rx: PCC CRC error count:                 %d",
		   ping_data.rx_metrics.rx_pcc_crc_error_count);
	desh_print("  rx: PDC CRC error count:                 %d",
		   ping_data.rx_metrics.rx_pdc_crc_error_count);
	desh_print("  rx: PDU decode error count:              %d",
		   ping_data.rx_metrics.rx_decode_error);
	desh_print("  rx: min RSSI %d, max RSSI %d", ping_data.rx_metrics.rx_rssi_low_level,
		   ping_data.rx_metrics.rx_rssi_high_level);
	desh_print("  rx: last TX pwr %d",
		   dect_common_utils_phy_tx_power_to_dbm(ping_data.rx_metrics.rx_phy_transmit_pwr));
	desh_print(
		"  rx: min pwr %d, max pwr %d",
		dect_common_utils_phy_tx_power_to_dbm(ping_data.rx_metrics.rx_phy_transmit_pwr_low),
		dect_common_utils_phy_tx_power_to_dbm(
			ping_data.rx_metrics.rx_phy_transmit_pwr_high));
	desh_print("  rx: min SNR %d, max SNR %d", ping_data.rx_metrics.rx_snr_low,
		   ping_data.rx_metrics.rx_snr_high);

	ret = dect_phy_ping_tx_request_results();
	if (ret) {
		desh_error("Failed (err =%d) to request results from the server.", ret);
		dect_phy_ping_cmd_done();
	} else {
		ping_data.client_data.tx_results_from_server_requested = true;
		desh_print("Requested ping results from the server...");
	}
	/* Clear results */
	struct dect_phy_ping_params *params = &(ping_data.cmd_params);

	ping_data.restarted_count = 0;
	dect_phy_ping_rx_metrics_reset(params);

	memset(&ping_data.tx_metrics, 0, sizeof(struct dect_phy_ping_tx_metrics));

	return ret;
}

static int dect_phy_ping_server_results_tx(char *result_str)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_ping_params *params = &(ping_data.cmd_params);
	struct nrf_modem_dect_phy_tx_rx_params tx_rx_param;

	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 0,
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001,
		.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF),
		.transmitter_identity_hi = (uint8_t)(current_settings->common.transmitter_id >> 8),
		.transmitter_identity_lo =
			(uint8_t)(current_settings->common.transmitter_id & 0xFF),
		.df_mcs = 1,
		.transmit_power = dect_phy_ctrl_utils_mdm_next_supported_phy_tx_power_get(
			ping_data.rx_metrics.rx_phy_transmit_pwr, params->channel),
		.receiver_identity_hi = (uint8_t)(ping_data.server_data.rx_last_tx_id >> 8),
		.receiver_identity_lo = (uint8_t)(ping_data.server_data.rx_last_tx_id & 0xFF),
		.feedback.format1.format = 0,
	};
	union nrf_modem_dect_phy_hdr phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
	dect_phy_ping_pdu_t ping_pdu;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN];
	int ret = 0;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx =
		time_now + (2 * US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));
	uint16_t bytes_to_send =
		DECT_PHY_PING_PDU_COMMON_PART_LEN + sizeof(dect_phy_ping_pdu_results_resp_data);

	/* Calculate the slot count needs */
	header.packet_length = dect_common_utils_phy_packet_length_calculate(
		bytes_to_send, header.packet_length_type, header.df_mcs);
	if (header.packet_length < 0) {
		desh_error("(%s): Phy pkt len calculation failed", (__func__));
		return -EINVAL;
	}
	memcpy(&phy_header.type_2, &header, sizeof(phy_header.type_2));

	ping_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP;
	ping_pdu.header.transmitter_id = current_settings->common.transmitter_id;
	ping_pdu.header.pwr_ctrl_expected_rssi_level_dbm =
		params->pwr_ctrl_pdu_expected_rx_rssi_level;
	strcpy(ping_pdu.message.results.results_str, result_str);
	dect_phy_ping_pdu_encode(encoded_data_to_send, &ping_pdu);

	tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	tx_op.carrier = params->channel;
	tx_op.data_size = bytes_to_send;
	tx_op.data = encoded_data_to_send;
	tx_op.lbt_period = 0;
	tx_op.lbt_rssi_threshold_max = current_settings->rssi_scan.busy_threshold;

	tx_op.network_id = current_settings->common.network_id;
	tx_op.phy_header = &phy_header;
	tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
	tx_op.handle = DECT_PHY_PING_RESULTS_RESP_TX_HANDLE;
	tx_op.start_time = first_possible_tx;

	tx_rx_param.tx = tx_op;
	tx_rx_param.rx = ping_data.server_data.server_rx_op;

	/* RX time is relative from TX end */
	tx_rx_param.rx.start_time = 3 * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS;

	ret = nrf_modem_dect_phy_tx_rx(&tx_rx_param);
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_tx failed %d (handle %d)\n", (__func__), ret,
			   tx_op.handle);
	} else {
		ping_data.server_data.rx_enabled_z_ticks = k_uptime_get();
		ping_data.server_data.on_going_rx_count++;

		desh_print("Server results TX scheduled: %d bytes (%d slots, MCS-%d).",
			   bytes_to_send, header.packet_length + 1, header.df_mcs);
	}

	return ret;
}

static int dect_phy_ping_server_ping_resp_tx(struct dect_phy_data_rcv_common_params *params,
					     dect_phy_ping_pdu_t *ping_req_pdu)
{
	struct nrf_modem_dect_phy_tx_rx_params tx_rx_param;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);
	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 0,
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001,
		.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF),
		.transmitter_identity_hi = (uint8_t)(current_settings->common.transmitter_id >> 8),
		.transmitter_identity_lo =
			(uint8_t)(current_settings->common.transmitter_id & 0xFF),
		.df_mcs = params->mcs,
		.transmit_power = dect_common_utils_dbm_to_phy_tx_power(params->rx_pwr_dbm),
		.receiver_identity_hi = (uint8_t)(ping_data.server_data.rx_last_tx_id >> 8),
		.receiver_identity_lo = (uint8_t)(ping_data.server_data.rx_last_tx_id & 0xFF),
		.feedback.format1.format = 0,
	};
	union nrf_modem_dect_phy_hdr phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
	dect_phy_ping_pdu_t ping_pdu;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN];
	int ret = 0;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx =
		time_now + (US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));
	uint16_t bytes_to_send = params->data_length;

	ping_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PING_RESPONSE;
	ping_pdu.header.transmitter_id = current_settings->common.transmitter_id;

	/* Calculate the slot count needs */
	header.packet_length = dect_common_utils_phy_packet_length_calculate(
		bytes_to_send, header.packet_length_type, header.df_mcs);
	if (header.packet_length < 0) {
		desh_error("(%s): Phy pkt len calculation failed", (__func__));
		return -EINVAL;
	}

	if (cmd_params->pwr_ctrl_automatic) {
		header.transmit_power = dect_common_utils_dbm_to_phy_tx_power(
			dect_phy_ping_auto_pwr_ctrl_target_pwr_calculate());
	}

	memcpy(&phy_header.type_2, &header, sizeof(phy_header.type_2));

	__ASSERT_NO_MSG(ping_req_pdu->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_REQUEST);

	ping_pdu.message.tx_data.seq_nbr = ping_req_pdu->message.tx_data.seq_nbr;
	ping_pdu.message.tx_data.payload_length = ping_req_pdu->message.tx_data.payload_length;
	ping_pdu.header.pwr_ctrl_expected_rssi_level_dbm =
		cmd_params->pwr_ctrl_pdu_expected_rx_rssi_level;

	memcpy(ping_pdu.message.tx_data.pdu_payload, ping_req_pdu->message.tx_data.pdu_payload,
	       ping_req_pdu->message.tx_data.payload_length);

	dect_phy_ping_pdu_encode(encoded_data_to_send, &ping_pdu);

	tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	tx_op.carrier = cmd_params->channel;
	tx_op.data_size = bytes_to_send;
	tx_op.data = encoded_data_to_send;
	tx_op.lbt_period = 0;
	tx_op.lbt_rssi_threshold_max = current_settings->rssi_scan.busy_threshold;

	tx_op.network_id = current_settings->common.network_id;
	tx_op.phy_header = &phy_header;
	tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
	tx_op.handle = DECT_PHY_PING_SERVER_TX_HANDLE;
	tx_op.start_time = first_possible_tx;
	tx_rx_param.tx = tx_op;

	tx_rx_param.rx = ping_data.server_data.server_rx_op;

	/* RX time is relative from TX end */
	tx_rx_param.rx.start_time = 3 * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS;

	ret = nrf_modem_dect_phy_tx_rx(&tx_rx_param);
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_tx_rx failed %d (tx handle %d)\n", (__func__),
			   ret, tx_op.handle);
	} else {
		ping_data.server_data.rx_enabled_z_ticks = k_uptime_get();
		ping_data.server_data.on_going_rx_count++;
	}
	return ret;
}

static void dect_phy_ping_server_report_local_and_tx_results(void)
{

	if (!ping_data.rx_metrics.rx_total_data_amount) {
		desh_print("Nothing received on server side - "
			   "no results to show.");
		goto clear_results;
	}

	int64_t elapsed_time_ms = ping_data.server_data.rx_last_data_received -
				  ping_data.server_data.rx_1st_data_received;
	double elapsed_time_secs = elapsed_time_ms / 1000;
	char results_str[DECT_PHY_PING_RESULTS_DATA_MAX_LEN];

	memset(results_str, 0, sizeof(results_str));

	sprintf(results_str, "\nping server operation with tx id %d:\n",
		ping_data.server_data.rx_last_tx_id);
	sprintf(results_str + strlen(results_str), "  elapsed time:               %.2f seconds\n",
		elapsed_time_secs);
	sprintf(results_str + strlen(results_str), "  tx: ping resp count:        %d\n",
		ping_data.tx_metrics.tx_total_ping_resp_count);
	sprintf(results_str + strlen(results_str), "  rx: ping req count:         %d\n",
		ping_data.rx_metrics.rx_total_ping_req_count);
	sprintf(results_str + strlen(results_str), "  rx: total amount of data:   %d bytes\n",
		ping_data.rx_metrics.rx_total_data_amount);
	sprintf(results_str + strlen(results_str), "  rx: PCC CRC error count:    %d\n",
		ping_data.rx_metrics.rx_pcc_crc_error_count);
	sprintf(results_str + strlen(results_str), "  rx: PDC CRC error count:    %d\n",
		ping_data.rx_metrics.rx_pdc_crc_error_count);
	sprintf(results_str + strlen(results_str), "  rx: out of sequence count:  %d\n",
		ping_data.rx_metrics.rx_out_of_seq_count);
	sprintf(results_str + strlen(results_str), "  rx: PDU decode error count: %d\n",
		ping_data.rx_metrics.rx_decode_error);
	sprintf(results_str + strlen(results_str), "  rx: min RSSI %d, max RSSI %d\n",
		ping_data.rx_metrics.rx_rssi_low_level, ping_data.rx_metrics.rx_rssi_high_level);
	sprintf(results_str + strlen(results_str), "  rx: last TX pwr %d\n",
		dect_common_utils_phy_tx_power_to_dbm(ping_data.rx_metrics.rx_phy_transmit_pwr));
	sprintf(results_str + strlen(results_str), "  rx: min pwr %d, max pwr %d\n",
		dect_common_utils_phy_tx_power_to_dbm(ping_data.rx_metrics.rx_phy_transmit_pwr_low),
		dect_common_utils_phy_tx_power_to_dbm(
			ping_data.rx_metrics.rx_phy_transmit_pwr_high));
	sprintf(results_str + strlen(results_str), "  rx: min SNR %d, max SNR %d\n",
		ping_data.rx_metrics.rx_snr_low, ping_data.rx_metrics.rx_snr_high);

	desh_print("%s", results_str);

	/* Send results to client: */
	if (dect_phy_ping_server_results_tx(results_str)) {
		desh_error("Cannot start sending server results");
	}

clear_results:
	/* Clear results */
	struct dect_phy_ping_params *params = &(ping_data.cmd_params);

	ping_data.restarted_count = 0;
	ping_data.server_data.rx_last_tx_id = 0;
	ping_data.server_data.rx_last_data_received = 0;
	ping_data.server_data.rx_1st_data_received = 0;

	dect_phy_ping_rx_metrics_reset(params);

	memset(&ping_data.tx_metrics, 0, sizeof(struct dect_phy_ping_tx_metrics));
}

void dect_phy_ping_mdm_op_completed(
	struct dect_phy_common_op_completed_params *mdm_completed_params)
{
	if (ping_data.on_going) {
		if (mdm_completed_params->handle == DECT_PHY_PING_CLIENT_TX_HANDLE) {
			if (mdm_completed_params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				/* TX was not done */
				dect_phy_ping_client_data_tx_total_data_amount_decrease();
				desh_print("ping tx failed.");
			} else {
				desh_print("ping sent (seq_nbr %d).",
					   ping_data.client_data.tx_next_seq_nbr);
			}
			/* We count both success and failures here */
			ping_data.tx_metrics.tx_total_ping_req_count++;

			/* Update header for new data ind toggle (only with HARQ) and
			 * for seq nbr in ping PDU
			 */
			if (ping_data.cmd_params.use_harq) {
				struct dect_phy_header_type2_format0_t *header =
					(void *)&(ping_data.client_data.tx_phy_header);

				header->df_new_data_indication_toggle =
					dect_phy_ping_harq_process_next_new_data_ind_get(
						DECT_HARQ_CLIENT);

				/* Update header to scheduler for a next round */
				dect_phy_api_scheduler_list_item_tx_phy_header_update_by_phy_handle(
					DECT_PHY_PING_CLIENT_TX_HANDLE,
					&(ping_data.client_data.tx_phy_header),
					DECT_PHY_HEADER_TYPE2);
			}

			uint8_t *encoded_data_to_send = ping_data.client_data.tx_data;
			uint8_t *seq_nbr_ptr = encoded_data_to_send + DECT_PHY_PING_PDU_HEADER_LEN;

			ping_data.client_data.tx_ping_resp_received = false;
			ping_data.client_data.tx_next_seq_nbr++;
			dect_common_utils_16bit_be_write(seq_nbr_ptr,
							 ping_data.client_data.tx_next_seq_nbr);

			if (!ping_data.client_data.tx_scheduler_intervals_done) {
				/* Modify seq nbr for the next round */
				dect_phy_api_scheduler_list_item_pdu_payload_update_by_phy_handle(
					DECT_PHY_PING_CLIENT_TX_HANDLE, encoded_data_to_send,
					ping_data.client_data.tx_data_len);
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PING_SERVER_TX_HANDLE) {
			if (mdm_completed_params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				/* TX was not done */
				desh_print("ping resp tx failed.");
			} else {
				desh_print("ping resp sent (seq_nbr %d).",
					   ping_data.server_data.rx_last_seq_nbr);
				ping_data.tx_metrics.tx_total_ping_resp_count++;
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PING_CLIENT_RX_HANDLE) {
			dect_phy_ping_rssi_done_evt_send();
			if (!ping_data.client_data.tx_ping_resp_received &&
			    !harq_processes[DECT_HARQ_CLIENT].rtx_ongoing) {
				desh_warn("ping timeout for seq_nbr %d",
					  ping_data.client_data.tx_next_seq_nbr - 1);
			}
			if (ping_data.client_data.tx_scheduler_intervals_done &&
			    !harq_processes[DECT_HARQ_CLIENT].rtx_ongoing &&
			    ping_data.cmd_params.ping_count ==
				    ping_data.tx_metrics.tx_total_ping_req_count) {
				dect_phy_ping_client_report_local_results_and_req_server_results(
					NULL);
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PING_CLIENT_RE_TX_HANDLE) {
			harq_processes[DECT_HARQ_CLIENT].rtx_ongoing = false;
			if (mdm_completed_params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				/* TX was not done */
				desh_warn("ping retransmission based on HARQ failed.");
			} else {
				desh_print("ping retransmission based on HARQ done.");
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PING_RESULTS_REQ_TX_HANDLE) {
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_print("PING_RESULT_REQ sent.");
			} else {
				desh_warn("PING_RESULT_REQ sending failed.");
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PING_RESULTS_RESP_TX_HANDLE) {
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_print("PING_RESULT_RESP sent.");
			} else {
				desh_warn("PING_RESULT_RESP sending failed.");
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PING_RESULTS_RESP_RX_HANDLE) {
			/* Client */
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_print("RX for ping results done.");
			} else {
				desh_print("RX for ping results failed.");
			}
			dect_phy_ping_cmd_done();
		} else if (mdm_completed_params->handle == DECT_PHY_PING_SERVER_RX_HANDLE) {
			int64_t z_ticks_rx_scheduled = ping_data.server_data.rx_enabled_z_ticks;
			int64_t z_delta_ms = k_uptime_delta(&z_ticks_rx_scheduled);
			uint64_t time_now = dect_app_modem_time_now();
			uint64_t first_possible_start;
			struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

			first_possible_start =
				time_now +
				US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us);

			dect_phy_ping_server_data_on_going_rx_count_decrease();

			/* Restart server if needed */
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				if (ping_data.cmd_params.role == DECT_PHY_COMMON_ROLE_SERVER &&
				    (ping_data.server_data.on_going_rx_count == 0 ||
				     z_delta_ms > 62000)) {
					struct dect_phy_ping_params copy_params =
						ping_data.cmd_params;

					dect_phy_ping_start(
						&copy_params, RESTART, first_possible_start);
				}
			} else {
				/* Let's restart only if failure is not happening very fast which
				 * usually means a fundamental failure and then it is better to
				 * quit.
				 */
				if (ping_data.cmd_params.role == DECT_PHY_COMMON_ROLE_SERVER &&
				    ping_data.server_data.on_going_rx_count == 0 &&
				    z_delta_ms > 500) {
					struct dect_phy_ping_params copy_params =
						ping_data.cmd_params;

					dect_phy_ping_start(
						&copy_params, RESTART, first_possible_start);
				} else {
					desh_error(
						"ping server RX failed - ping server RX stopped.");
					dect_phy_ping_cmd_done();
				}
			}
		}
	}
}

/**************************************************************************************************/

K_MSGQ_DEFINE(dect_phy_ping_op_event_msgq, sizeof(struct dect_phy_common_op_event_msgq_item), 10,
	      4);

/**************************************************************************************************/

#define DECT_PHY_PING_THREAD_STACK_SIZE 6144
#define DECT_PHY_PING_THREAD_PRIORITY	5

static void dect_phy_ping_thread_fn(void)
{
	struct dect_phy_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_ping_op_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_PHY_PING_EVENT_RSSI_COUNT_DONE: {
			dect_phy_ping_rssi_scan_print();
			break;
		}
		case DECT_PHY_PING_EVENT_CLIENT_SCHEDULER_PINGING_DONE: {
			desh_print("pinging done by scheduler");
			ping_data.client_data.tx_scheduler_intervals_done = true;
			break;
		}
		case DECT_PHY_PING_EVENT_CMD_DONE: {
			dect_phy_ping_cmd_done();
			break;
		}
		case DECT_PHY_PING_EVENT_MDM_OP_COMPLETED: {
			struct dect_phy_common_op_completed_params *params =
				(struct dect_phy_common_op_completed_params *)event.data;

			if (params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				if (dect_phy_ping_handle_is(params->handle)) {
					dect_phy_ping_mdm_op_completed(params);
				} else if (params->handle == DECT_HARQ_FEEDBACK_TX_HANDLE) {
					desh_print("HARQ feedback sent.");
				}
			} else if (params->status != NRF_MODEM_DECT_PHY_ERR_OP_CANCELED) {
				char tmp_str[128] = {0};

				dect_common_utils_modem_phy_err_to_string(
					params->status, params->temperature, tmp_str);
				if (dect_phy_ping_handle_is(params->handle)) {
					if (params->status ==
						NRF_MODEM_DECT_PHY_ERR_LBT_CHANNEL_BUSY) {
						desh_warn("%s: cannot TX ping due to LBT, "
							"channel was busy", __func__);
						ping_data.tx_metrics.tx_failed_due_to_lbt++;
					} else {
						desh_warn("%s: ping modem operation "
							"failed: %s, handle %d",
							__func__, tmp_str, params->handle);
					}
					dect_phy_ping_mdm_op_completed(params);
				} else {
					desh_error("%s: operation (handle %d) failed with "
						   "status: %s",
						   __func__, params->handle, tmp_str);
				}
			}
			break;
		}
		case DECT_PHY_PING_EVENT_SERVER_REPORT: {
			dect_phy_ping_server_report_local_and_tx_results();
			break;
		}
		case DECT_PHY_PING_EVENT_RX_PCC_CRC_ERROR: {
			struct dect_phy_common_op_pcc_crc_fail_params *params =
				(struct dect_phy_common_op_pcc_crc_fail_params *)event.data;
			struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);

			if (ping_data.on_going) {
				dect_phy_ping_rx_on_pcc_crc_failure();
			}

			if (cmd_params->debugs) {
				/* Do not print too often error prints so that
				 * ping thread is not flooded due to them.
				 */

				int64_t z_ticks_last_pcc_error_print =
					ping_data.rx_metrics.rx_last_pcc_error_print_zticks;
				int64_t since_last_print_ms =
					k_uptime_delta(&z_ticks_last_pcc_error_print);

				if (since_last_print_ms >= 100) {
					ping_data.rx_metrics.rx_last_pcc_error_print_zticks =
						z_ticks_last_pcc_error_print;
					desh_warn("PING: RX PCC CRC error (time %llu): SNR %d, "
						  "RSSI-2 %d (%d dBm)",
						  params->time, params->crc_failure.snr,
						  params->crc_failure.rssi_2,
						  (params->crc_failure.rssi_2 / 2));
				}
			}
			break;
		}
		case DECT_PHY_PING_EVENT_RX_PDC_CRC_ERROR: {
			struct dect_phy_common_op_pdc_crc_fail_params *params =
				(struct dect_phy_common_op_pdc_crc_fail_params *)event.data;
			struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);

			if (ping_data.on_going) {
				dect_phy_ping_rx_on_pdc_crc_failure();
			}
			if (cmd_params->debugs) {
				desh_warn("PING: RX PDC CRC error (time %llu): SNR %d, RSSI-2 %d "
					  "(%d dBm)",
					  params->time, params->crc_failure.snr,
					  params->crc_failure.rssi_2,
					  (params->crc_failure.rssi_2 / 2));
			}
			break;
		}
		case DECT_PHY_PING_EVENT_RX_PCC: {
			struct dect_phy_common_op_pcc_rcv_params *params =
				(struct dect_phy_common_op_pcc_rcv_params *)event.data;
			struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);
			struct dect_phy_ctrl_field_common *phy_h = (void *)&(params->phy_header);
			char tmp_str[128] = {0};
			uint8_t short_nw_id;
			uint8_t *p_ptr = (uint8_t *)&(params->phy_header);
			int16_t pcc_rx_pwr_dbm =
				dect_common_utils_phy_tx_power_to_dbm(phy_h->transmit_power);
			uint16_t transmitter_id, receiver_id;
			int16_t rssi_level = params->pcc_status.rssi_2 / 2;

			dect_common_utils_modem_phy_header_status_to_string(
				params->pcc_status.header_status, tmp_str);
			p_ptr++;
			short_nw_id = *p_ptr++;
			transmitter_id = sys_get_be16(p_ptr);
			p_ptr = p_ptr + 3;
			receiver_id = sys_get_be16(p_ptr);

			/* Store min/max RSSI/pwr/snr and last tx pwr as seen on PCC RX */
			if (rssi_level > ping_data.rx_metrics.rx_rssi_high_level) {
				ping_data.rx_metrics.rx_rssi_high_level = rssi_level;
			}
			if (rssi_level < ping_data.rx_metrics.rx_rssi_low_level) {
				ping_data.rx_metrics.rx_rssi_low_level = rssi_level;
			}
			ping_data.rx_metrics.rx_latest_rssi_level = rssi_level;
			ping_data.rx_metrics.rx_last_pcc_mcs = phy_h->df_mcs;

			ping_data.rx_metrics.rx_last_tx_id_from_pcc = transmitter_id;

			ping_data.rx_metrics.rx_phy_transmit_pwr = phy_h->transmit_power;
			if (phy_h->transmit_power > ping_data.rx_metrics.rx_phy_transmit_pwr_high) {
				ping_data.rx_metrics.rx_phy_transmit_pwr_high =
					phy_h->transmit_power;
			}
			if (phy_h->transmit_power < ping_data.rx_metrics.rx_phy_transmit_pwr_low) {
				ping_data.rx_metrics.rx_phy_transmit_pwr_low =
					phy_h->transmit_power;
			}
			if (params->pcc_status.snr > ping_data.rx_metrics.rx_snr_high) {
				ping_data.rx_metrics.rx_snr_high = params->pcc_status.snr;
			}
			if (params->pcc_status.snr < ping_data.rx_metrics.rx_snr_low) {
				ping_data.rx_metrics.rx_snr_low = params->pcc_status.snr;
			}

			if (cmd_params->debugs) {
				desh_print("PCC received (time %llu, handle %d): "
					   "status: \"%s\", snr %d, "
					   "RSSI-2 %d (RSSI %d), stf_start_time %llu",
					   params->time, params->pcc_status.handle,
					   tmp_str, params->pcc_status.snr,
					   params->pcc_status.rssi_2, rssi_level,
					   params->pcc_status.stf_start_time);
				desh_print("  phy header:");
				desh_print("    short nw id %d, transmitter id %d", short_nw_id,
					   transmitter_id);
				desh_print("    receiver id: %d", receiver_id);
				desh_print("    len %d, MCS %d, TX pwr: %d dBm",
					   phy_h->packet_length, phy_h->df_mcs, pcc_rx_pwr_dbm);
			}

			/* Handle HARQ feedback */
			if (!(cmd_params->use_harq &&
			      params->pcc_status.header_status ==
				      NRF_MODEM_DECT_PHY_HDR_STATUS_VALID &&
			      params->pcc_status.phy_type == DECT_PHY_HEADER_TYPE2)) {
				break;
			}
			struct dect_phy_header_type2_format0_t *header =
				(void *)&(params->phy_header);

			if (!(header->format == DECT_PHY_HEADER_FORMAT_001 &&
			      header->feedback.format1.format == 1)) {
				break;
			}

			if (header->feedback.format1.harq_process_number0 == DECT_HARQ_CLIENT) {
				char color[10];

				sprintf(color, "%s", ANSI_COLOR_GREEN);
				if (header->feedback.format1.transmission_feedback0) {
					sprintf(color, "%s", ANSI_COLOR_GREEN);
				} else {
					sprintf(color, "%s", ANSI_COLOR_RED);
				}
				desh_print("PCC: HARQ resp %s%s%s for DECT_HARQ_CLIENT process.",
					color,
					(header->feedback.format1.transmission_feedback0)
					   ? "ACK"
					   : "NACK",
					ANSI_RESET_ALL);
				if (transmitter_id != (uint16_t)harq_processes[DECT_HARQ_CLIENT]
							      .res_data.peer_transmitter_id) {
					desh_error("PCC: DECT_HARQ_CLIENT: HARQ resp from %d"
						   " AND thus not from our peer %d.",
						   transmitter_id,
						   harq_processes[DECT_HARQ_CLIENT]
							   .res_data.peer_transmitter_id);

				} else if (harq_processes[DECT_HARQ_CLIENT].process_in_use) {
					if (header->feedback.format1.transmission_feedback0) {
						/* ACK: clear the HARQ process resources  */
						dect_phy_ping_harq_process_round_end(
							DECT_HARQ_CLIENT);
					} else {
						/* NACK: stop current RX, TX a retransmit,
						 * restart RX for a remaining duration.
						 */
						(void)nrf_modem_dect_phy_cancel(
							DECT_PHY_PING_CLIENT_RX_HANDLE);
						dect_phy_ping_harq_client_nak_handle();
					}
				}
			} else {
				desh_warn("Unsupported process nbr %d received in HARQ feedback",
					  header->feedback.format1.harq_process_number0);
			}

			break;
		}

		case DECT_PHY_PING_EVENT_RX_PDC_DATA: {
			struct dect_phy_commmon_op_pdc_rcv_params *params =
				(struct dect_phy_commmon_op_pdc_rcv_params *)event.data;
			struct dect_phy_data_rcv_common_params rcv_params = {
				.time = params->time,
				.snr = params->rx_status.snr,
				.rssi = params->rx_status.rssi_2 / 2,
				.rx_rssi_dbm = params->rx_rssi_level_dbm,
				.mcs = params->rx_mcs,
				.rx_pwr_dbm = params->rx_pwr_dbm,
				.data_length = params->data_length,
				.data = params->data,
			};
			struct nrf_modem_dect_phy_pdc_event *p_rx_status = &(params->rx_status);
			int16_t rssi_level = p_rx_status->rssi_2 / 2;

			desh_print("PDC received (time %llu, handle %d): snr %d, RSSI-2 %d "
				   "(RSSI %d), len %d",
				   params->time, p_rx_status->handle,
				   p_rx_status->snr, p_rx_status->rssi_2, rssi_level,
				   params->data_length);

			if (params->data_length) {
				uint8_t *pdu_type_ptr = (uint8_t *)params->data;
				uint8_t pdu_type = *pdu_type_ptr & DECT_COMMON_UTILS_BIT_MASK_7BIT;

				if (pdu_type == DECT_MAC_MESSAGE_TYPE_PING_REQUEST ||
				    pdu_type == DECT_MAC_MESSAGE_TYPE_PING_RESPONSE ||
				    pdu_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ ||
				    pdu_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP ||
				    pdu_type == DECT_MAC_MESSAGE_TYPE_PING_HARQ_FEEDBACK) {
					if (ping_data.on_going) {
						dect_phy_ping_rx_pdc_data_handle(&rcv_params);
					} else {
						desh_print("ping command TX data received: ignored "
							   "- no ping command running.");
					}
				} else {
					unsigned char hex_data[128];
					int i;
					char snum[64] = {0};

					for (i = 0; i < 64 && i < params->data_length; i++) {
						sprintf(&hex_data[i], "%02x ", params->data[i]);
					}
					hex_data[i + 1] = '\0';
					desh_warn("ping: received unknown data (type: %s), len %d, "
						  "hex data: %s\n",
						  dect_common_pdu_message_type_to_string(pdu_type,
											 snum),
						  params->data_length, hex_data);
				}
			}
			break;
		}
		case DECT_PHY_PING_EVENT_HARQ_PAYLOAD_STORED: {
			enum dect_harq_user *harq_user = (enum dect_harq_user *)event.data;

			dect_phy_ping_harq_process_round_start(*harq_user);
			break;
		}

		default:
			desh_warn("DECT PING: Unknown event %d received", event.id);
			break;
		}
		k_free(event.data);
	}
}

K_THREAD_DEFINE(dect_phy_ping_th, DECT_PHY_PING_THREAD_STACK_SIZE, dect_phy_ping_thread_fn, NULL,
		NULL, NULL, K_PRIO_PREEMPT(DECT_PHY_PING_THREAD_PRIORITY), 0, 0);

/**************************************************************************************************/

static int dect_phy_ping_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);

	event.id = event_id;
	ret = k_msgq_put(&dect_phy_ping_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

static int dect_phy_ping_msgq_non_data_op_add(uint16_t event_id)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.id = event_id;
	event.data = NULL;
	ret = k_msgq_put(&dect_phy_ping_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

void dect_phy_ping_mdm_evt_handler(const struct nrf_modem_dect_phy_event *evt)
{
	dect_app_modem_time_save(&evt->time);

	switch (evt->id) {
	case NRF_MODEM_DECT_PHY_EVT_COMPLETED:
		dect_phy_ping_mdm_op_complete_cb(&evt->op_complete, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC:
		dect_phy_ping_mdm_pcc_cb(&evt->pcc, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC_ERROR:
		dect_phy_ping_mdm_pcc_crc_failure_cb(&evt->pcc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC:
		dect_phy_ping_mdm_pdc_cb(&evt->pdc, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC_ERROR:
		dect_phy_ping_mdm_pdc_crc_failure_cb(&evt->pdc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_RSSI:
		dect_phy_ping_mdm_rssi_cb(&evt->rssi);
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

static void dect_phy_ping_mdm_init(void)
{
	int ret = nrf_modem_dect_phy_event_handler_set(dect_phy_ping_mdm_evt_handler);

	if (ret) {
		printk("nrf_modem_dect_phy_event_handler_set returned: %i\n", ret);
	}
}

int dect_phy_ping_cmd_handle(struct dect_phy_ping_params *params)
{
	int ret;

	if (ping_data.on_going) {
		desh_error("ping command already running");
		return -1;
	}
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_start;

	first_possible_start =
		time_now + (3 * US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));

	dect_phy_ping_mdm_init();

	dect_phy_ping_rssi_curr_data_reset();

	ret = dect_phy_ping_start(params, START, first_possible_start);
	if (ret) {
		ping_data.on_going = false;
	}
	return ret;
}

void dect_phy_ping_cmd_stop(void)
{
	if (!ping_data.on_going) {
		desh_print("No ping command running - nothing to stop.");
		return;
	}
	if (ping_data.cmd_params.role == DECT_PHY_COMMON_ROLE_CLIENT) {
		int err = 0;

		err = dect_phy_ping_client_report_local_results_and_req_server_results(NULL);
		if (err) {
			dect_phy_ping_cmd_done();
		}
	} else {
		dect_phy_ping_msgq_non_data_op_add(DECT_PHY_PING_EVENT_SERVER_REPORT);
		dect_phy_ping_server_data_on_going_rx_count_decrease();
		dect_phy_ping_msgq_non_data_op_add(DECT_PHY_PING_EVENT_CMD_DONE);
	}
}

/**************************************************************************************************/

void dect_phy_ping_rx_on_pcc_crc_failure(void)
{
	if (!ping_data.on_going) {
		desh_error("receiving ping data but ping is not running.");
		return;
	}
	ping_data.rx_metrics.rx_pcc_crc_error_count++;
}

void dect_phy_ping_rx_on_pdc_crc_failure(void)
{
	if (!ping_data.on_going) {
		desh_error("receiving ping data but ping is not running.");
		return;
	}
	ping_data.rx_metrics.rx_pdc_crc_error_count++;
}

static int dect_phy_ping_rx_pdc_data_handle(struct dect_phy_data_rcv_common_params *params)
{
	dect_phy_ping_pdu_t pdu;
	int ret;

	if (!ping_data.on_going) {
		desh_error("receiving ping data but ping is not running.");
		return -1;
	}

	ret = dect_phy_ping_pdu_decode(&pdu, (const uint8_t *)params->data);
	if (ret) {
		ping_data.rx_metrics.rx_decode_error++;
		desh_error("dect_phy_ping_pdu_decode failed: %d", ret);
		return -EBADMSG;
	}
	dect_phy_pdu_utils_ping_print(&pdu);
	ping_data.rx_metrics.rx_pdu_expected_rssi = pdu.header.pwr_ctrl_expected_rssi_level_dbm;

	if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PING_REQUEST) {
		dect_phy_ping_rssi_done_evt_send();
		if (ping_data.rx_metrics.rx_total_data_amount == 0) {
			ping_data.server_data.rx_1st_data_received = k_uptime_get();
			ping_data.server_data.rx_last_seq_nbr = pdu.message.tx_data.seq_nbr;
		} else {
			if (pdu.message.tx_data.seq_nbr !=
			    (ping_data.server_data.rx_last_seq_nbr + 1)) {
				ping_data.rx_metrics.rx_out_of_seq_count++;
				desh_warn("Out of seq in RX: rx_out_of_seq_count %d, "
					  "pdu.seq_nbr %d, "
					  "ping_data.server_data.rx_last_seq_nbr %d",
					  ping_data.rx_metrics.rx_out_of_seq_count,
					  pdu.message.tx_data.seq_nbr,
					  ping_data.server_data.rx_last_seq_nbr);
			}
		}
		ping_data.rx_metrics.rx_total_data_amount += params->data_length;
		ping_data.rx_metrics.rx_total_ping_req_count++;
		ping_data.server_data.rx_last_seq_nbr = pdu.message.tx_data.seq_nbr;
		ping_data.server_data.rx_last_tx_id = pdu.header.transmitter_id;
		ping_data.server_data.rx_last_data_received = k_uptime_get();

		ret = dect_phy_ping_server_ping_resp_tx(params, &pdu);
		if (ret) {
			desh_error("Cannot send ping response: err %d", ret);
		}
	} else if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESPONSE) {
		struct dect_phy_ping_params *cmd_params = &(ping_data.cmd_params);

		dect_phy_ping_rssi_done_evt_send();

		if (cmd_params->pwr_ctrl_automatic) {
			struct dect_phy_ctrl_field_common *phy_h =
				(void *)&(ping_data.client_data.tx_phy_header);

			phy_h->transmit_power = dect_common_utils_dbm_to_phy_tx_power(
				dect_phy_ping_auto_pwr_ctrl_target_pwr_calculate());

			/* Update header to scheduler */
			dect_phy_api_scheduler_list_item_tx_phy_header_update_by_phy_handle(
				DECT_PHY_PING_CLIENT_TX_HANDLE,
				&(ping_data.client_data.tx_phy_header), DECT_PHY_HEADER_TYPE2);
		}

		if (pdu.message.tx_data.seq_nbr == (ping_data.client_data.tx_next_seq_nbr - 1)) {
			char tmp_str[128] = {0};

			if (ping_data.client_data.tx_ping_req_tx_scheduled_mdm_ticks > 0) {
				/* Calculate RTT */
				int64_t rtt_ms = MODEM_TICKS_TO_MS(
					params->time -
					ping_data.client_data.tx_ping_req_tx_scheduled_mdm_ticks);

				sprintf(tmp_str, "RTT: %d msec", (uint32_t)rtt_ms);
			} else {
				sprintf(tmp_str, "RTT: not known");
			}

			desh_print("ping response for seq_nbr %d (%s)", pdu.message.tx_data.seq_nbr,
				   tmp_str);
			ping_data.client_data.tx_ping_resp_received = true;

		} else {
			desh_warn("ping response for unexpected seq_nbr %d (expected: %d)",
				  pdu.message.tx_data.seq_nbr,
				  (ping_data.client_data.tx_next_seq_nbr - 1));
			ping_data.client_data.tx_ping_req_tx_scheduled_mdm_ticks = -1;
			ping_data.rx_metrics.rx_out_of_seq_count++;
		}
		ping_data.rx_metrics.rx_total_data_amount += params->data_length;
		ping_data.rx_metrics.rx_total_ping_resp_count++;
	} else if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ) {
		if (pdu.header.transmitter_id == ping_data.server_data.rx_last_tx_id) {
			desh_print("PING_RESULT_REQ received from tx id %d",
				pdu.header.transmitter_id);
			dect_phy_ping_server_report_local_and_tx_results();
		} else if (pdu.header.transmitter_id ==
				ping_data.rx_metrics.rx_last_tx_id_from_pcc) {
			desh_warn("PING_RESULT_REQ received from tx id %d - but no perf "
				  "data received from there.\n"
				  "However, we have seen a PCC lastly from this tx id %d - "
				  "so sending results to there.\n"
				  "Please, check MCS and/or TX PWR on a client side.",
				  pdu.header.transmitter_id,
				  ping_data.rx_metrics.rx_last_tx_id_from_pcc);
			ping_data.server_data.rx_last_tx_id =
				ping_data.rx_metrics.rx_last_tx_id_from_pcc;
			dect_phy_ping_server_report_local_and_tx_results();
		} else {
			desh_warn("PING_RESULTS_REQ received from tx id %d - but no ping "
				  "session with it (last seen tx id %d)",
				  pdu.header.transmitter_id, ping_data.server_data.rx_last_tx_id);
		}
	} else if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP) {
		ping_data.client_data.tx_results_from_server_requested = false;
		desh_print("Server results received.");
		dect_phy_ping_cmd_done();
	} else if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PING_HARQ_FEEDBACK) {
		desh_print("HARQ feedback received.");
	} else {
		desh_warn("type %d", pdu.header.message_type);
	}
	return 0;
}

/**************************************************************************************************/

static int dect_phy_ping_init(void)
{
	uint8_t *harq_payload1 = k_calloc(DECT_DATA_MAX_LEN, sizeof(uint8_t));
	uint8_t *harq_payload2 = k_calloc(DECT_DATA_MAX_LEN, sizeof(uint8_t));

	if (harq_payload1 == NULL || harq_payload2 == NULL) {
		desh_error("(%s): FATAL: cannot allocate memory for HARQ based reTX  payload",
			   (__func__));
	}

	dect_phy_ping_data_reset();

	harq_processes[DECT_HARQ_CLIENT].sche_list_item.sched_config.tx.encoded_payload_pdu =
		harq_payload1;
	harq_processes[DECT_HARQ_BEACON].sche_list_item.sched_config.tx.encoded_payload_pdu =
		harq_payload2;

	return 0;
}

SYS_INIT(dect_phy_ping_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
