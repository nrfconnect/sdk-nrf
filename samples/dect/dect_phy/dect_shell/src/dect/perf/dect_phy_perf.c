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

#include "dect_phy_perf_pdu.h"
#include "dect_phy_perf.h"

BUILD_ASSERT((sizeof(dect_phy_perf_pdu_t) == DECT_DATA_MAX_LEN),
	     "dect_phy_perf_pdu_t must be 700 bytes");

#define DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT	   8
#define DECT_PHY_PERF_HARQ_TX_PROCESS_TIMEOUT_SECS 2
#define DECT_PHY_PERF_CLIENT_TX_WINDOW_COUNT	   7

/**************************************************************************************************/
struct dect_phy_perf_harq_tx_process_info {
	bool process_in_use;
	bool usable;
	bool next_new_data_ind; /* Toggle */
	uint8_t process_nbr;
	uint16_t seq_nbr;
	uint32_t phy_op_handle;
	uint64_t time_when_reserved;
};

struct dect_phy_perf_tx_metrics {
	uint32_t tx_total_data_amount;
	uint32_t tx_total_pkt_count;
	uint32_t tx_harq_timeout_count;
};

struct dect_phy_perf_rx_metrics {
	int64_t rx_enabled_z_ticks;
	uint16_t rx_req_count_on_going;

	uint32_t rx_total_data_amount;
	uint32_t rx_total_pkt_count;
	int64_t rx_1st_data_received;
	int64_t rx_last_data_received;
	int64_t rx_last_pcc_received;
	uint8_t rx_last_pcc_phy_pwr;
	uint16_t rx_last_seq_nbr;
	uint16_t rx_last_tx_id;
	uint16_t rx_last_tx_id_from_pcc;

	uint32_t rx_pcc_crc_error_count;
	uint32_t rx_pdc_crc_error_count;
	uint32_t rx_out_of_seq_count;
	uint32_t rx_decode_error;
	uint32_t rx_restarted_count;
	int8_t rx_rssi_high_level;
	int8_t rx_rssi_low_level;
	uint8_t rx_phy_transmit_pwr_low;
	uint8_t rx_phy_transmit_pwr_high;
	int16_t rx_snr_low;
	int16_t rx_snr_high;
	uint8_t rx_latest_mcs;
	uint8_t rx_testing_mcs;

	int64_t rx_last_pcc_print_zticks;

	uint32_t harq_ack_rx_count;
	uint32_t harq_nack_rx_count;
	uint32_t harq_reset_nack_rx_count; /* feedback6 */
};

struct dect_phy_perf_client_data {
	bool tx_results_from_server_requested;
	int64_t tx_last_scheduled_mdm_op_start_time_mdm_ticks;
	uint16_t tx_last_round_starting_handle;
	uint16_t tx_harq_feedback_rx_last_round_middle_handle;
	uint16_t tx_last_seq_nbr;
	uint16_t tx_data_len;
	uint8_t tx_data[DECT_DATA_MAX_LEN]; /* max data size in bytes when MCS4 + 4 slots */

	struct dect_phy_perf_harq_tx_process_info
		tx_harq_processes[DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT];

	union nrf_modem_dect_phy_hdr tx_phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
	struct nrf_modem_dect_phy_rx_params rx_op; /* For HARQ receiving feedback */
};

struct dect_phy_perf_server_data {
	bool rx_results_sent;

	/* HARQ data */
	/* For sending HARQ feedback for RX data when requested by the client in a header */
	struct dect_phy_common_harq_feedback_data harq_feedback_data;
};

static struct dect_phy_perf_data {
	/* Common data  */
	bool perf_ongoing;
	int64_t time_started_ms;

	struct dect_phy_perf_params cmd_params;

	struct dect_phy_perf_rx_metrics rx_metrics;
	struct dect_phy_perf_tx_metrics tx_metrics;

	/* Server side */
	struct dect_phy_perf_server_data server_data;

	/* Client side TX */
	struct dect_phy_perf_client_data client_data;
} perf_data;

/**************************************************************************************************/

struct dect_phy_perf_op_data {
	bool in_use;
	uint32_t mdm_handle;
	int64_t op_start_time_mdm_ticks;
};

/**************************************************************************************************/

static void dect_phy_perf_rx_on_pcc_crc_failure(void);
static void dect_phy_perf_rx_on_pdc_crc_failure(void);
static int dect_phy_perf_rx_pdc_data_handle(struct dect_phy_data_rcv_common_params *params);

/**************************************************************************************************/

#define DECT_PHY_PERF_EVENT_MDM_OP_COMPLETED  1
#define DECT_PHY_PERF_EVENT_SERVER_REPORT     2
#define DECT_PHY_PERF_EVENT_RX_PDC_DATA	      3
#define DECT_PHY_PERF_EVENT_RX_PCC_CRC_ERROR  4
#define DECT_PHY_PERF_EVENT_RX_PDC_CRC_ERROR  5
#define DECT_PHY_PERF_EVENT_RX_PCC	      6
#define DECT_PHY_PERF_EVENT_HARQ_TX_WIN_TIMER 7
#define DECT_PHY_PERF_EVENT_CMD_DONE	      8

static int dect_phy_perf_msgq_non_data_op_add(uint16_t event_id);
static int dect_phy_perf_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size);

/**************************************************************************************************/

static void dect_phy_perf_client_tx_with_harq_timer_handler(struct k_timer *timer_id)
{
	dect_phy_perf_msgq_non_data_op_add(DECT_PHY_PERF_EVENT_HARQ_TX_WIN_TIMER);
}

K_TIMER_DEFINE(harq_tx_window_timer, dect_phy_perf_client_tx_with_harq_timer_handler, NULL);

/**************************************************************************************************/

static double dect_phy_perf_calculate_throughput(uint32_t byte_count, int64_t used_time_ms)
{
	double throughput = 8 * 1000 * ((double)byte_count / used_time_ms);

	return throughput;
}

/**************************************************************************************************/

static void dect_phy_perf_server_data_ongoing_rx_count_decrease(void)
{
	if (perf_data.rx_metrics.rx_req_count_on_going > 0) {
		perf_data.rx_metrics.rx_req_count_on_going--;
	}
}

/**************************************************************************************************/

static void dect_phy_perf_data_rx_metrics_init(void)
{
	memset(&perf_data.rx_metrics, 0, sizeof(perf_data.rx_metrics));
	perf_data.rx_metrics.rx_phy_transmit_pwr_high = 0; /* Bin: 0000 => -40dBm */
	perf_data.rx_metrics.rx_phy_transmit_pwr_low = 15; /* Bin: 1111 => 23dBm */
	perf_data.rx_metrics.rx_rssi_high_level = -127;
	perf_data.rx_metrics.rx_rssi_low_level = 1;
	perf_data.rx_metrics.rx_snr_low = 9999;
	perf_data.rx_metrics.rx_snr_high = -9999;
}

/**************************************************************************************************/

static void dect_phy_perf_prefill_server_rx_harq_feedback_data(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_perf_params *cmd_params = &(perf_data.cmd_params);

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
	dect_phy_perf_pdu_t tx_data_pdu = {
		.header.message_type = DECT_MAC_MESSAGE_TYPE_PERF_HARQ_FEEDBACK,
		.header.transmitter_id = current_settings->common.transmitter_id,
	};

	dect_phy_perf_pdu_encode(perf_data.server_data.harq_feedback_data.encoded_data_to_send,
				 &tx_data_pdu);

	harq_tx_params.data = perf_data.server_data.harq_feedback_data.encoded_data_to_send;
	perf_data.server_data.harq_feedback_data.params = harq_tx_params;
	perf_data.server_data.harq_feedback_data.header = header;
}

/**************************************************************************************************/

static int dect_phy_perf_server_rx_start(uint64_t start_time)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	struct dect_phy_perf_params *params = &(perf_data.cmd_params);
	struct nrf_modem_dect_phy_rx_params rx_op = {
		.handle = DECT_PHY_PERF_SERVER_RX_HANDLE,
		.start_time = start_time,
		.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
		.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
		.carrier = params->channel,
		.network_id = current_settings->common.network_id,
		.rssi_level = params->expected_rx_rssi_level,
		.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
	};
	int ret;
	uint64_t used_duration_mdm_ticks;

	rx_op.filter.is_short_network_id_used = true;
	rx_op.filter.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF);
	rx_op.filter.receiver_identity = current_settings->common.transmitter_id;

	if (params->use_harq) {
		dect_phy_perf_prefill_server_rx_harq_feedback_data();
		rx_op.mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT; /* We need to send HARQ
								      * feedback
								      */
	}

	if (params->duration_secs == DECT_PHY_PERF_DURATION_INFINITE) {
		used_duration_mdm_ticks = UINT32_MAX;
	} else {
		used_duration_mdm_ticks = SECONDS_TO_MODEM_TICKS(params->duration_secs);
	}

	if (used_duration_mdm_ticks > UINT32_MAX) {
		used_duration_mdm_ticks = UINT32_MAX;
	}
	rx_op.duration = used_duration_mdm_ticks;

	ret = dect_phy_common_rx_op(&rx_op);
	if (ret) {
		printk("nrf_modem_dect_phy_rx failed: ret %d, handle %d\n", ret, rx_op.handle);
		perf_data.perf_ongoing = false;
	} else {
		perf_data.rx_metrics.rx_enabled_z_ticks = k_uptime_get();
		perf_data.rx_metrics.rx_req_count_on_going++;
	}
	return ret;
}

/**************************************************************************************************/

static void dect_phy_perf_tx_total_data_decrease(void)
{
	if (perf_data.tx_metrics.tx_total_data_amount >= perf_data.client_data.tx_op.data_size) {
		/* TX was not done */
		perf_data.tx_metrics.tx_total_data_amount -= perf_data.client_data.tx_op.data_size;
	}
}

static void dect_phy_perf_harq_tx_process_table_init(uint8_t tx_harq_process_max_nbr)
{
	int i;

	memset(perf_data.client_data.tx_harq_processes, 0,
		sizeof(perf_data.client_data.tx_harq_processes));

	for (i = 0; i < DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT; i++) {
		perf_data.client_data.tx_harq_processes[i].process_nbr = i;
		perf_data.client_data.tx_harq_processes[i].next_new_data_ind = true;
		/* perf_data.client_data.tx_harq_processes[i].time_when_reserved = 0; */
		/* perf_data.client_data.tx_harq_processes[i].usable = false; */
		if (i < tx_harq_process_max_nbr) {
			perf_data.client_data.tx_harq_processes[i].usable = true;
		}
	}
}

static void dect_phy_perf_harq_tx_process_release(uint8_t process_nbr)
{
	__ASSERT_NO_MSG(process_nbr < DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT);
	perf_data.client_data.tx_harq_processes[process_nbr].process_in_use = false;
	perf_data.client_data.tx_harq_processes[process_nbr].time_when_reserved = 0;
}

static void dect_phy_perf_harq_tx_process_cleanup(void)
{
	for (int i = 0; i < DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT; i++) {
		if (perf_data.client_data.tx_harq_processes[i].process_in_use) {
			int64_t ref_time_when_reserved =
				perf_data.client_data.tx_harq_processes[i].time_when_reserved;
			int64_t elapsed_time_ms;

			__ASSERT_NO_MSG(ref_time_when_reserved != 0);
			elapsed_time_ms = k_uptime_delta(&ref_time_when_reserved);

			if ((elapsed_time_ms / 1000) >=
			    DECT_PHY_PERF_HARQ_TX_PROCESS_TIMEOUT_SECS) {
				perf_data.tx_metrics.tx_harq_timeout_count++;
				dect_phy_perf_harq_tx_process_release(
					perf_data.client_data.tx_harq_processes[i].process_nbr);
			}
		}
	}
}

static bool
dect_phy_perf_harq_tx_process_reserve(struct dect_phy_perf_harq_tx_process_info **harq_tx_process)
{
	bool return_value = false;
	int i;

	*harq_tx_process = NULL;

	/* 1st cleanup old ones */
	dect_phy_perf_harq_tx_process_cleanup();

	/* And now, get the 1st free slot */
	for (i = 0; i < DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT; i++) {
		if (!perf_data.client_data.tx_harq_processes[i].process_in_use &&
		    perf_data.client_data.tx_harq_processes[i].usable) {
			return_value = true;
			perf_data.client_data.tx_harq_processes[i].process_in_use = true;
			perf_data.client_data.tx_harq_processes[i].time_when_reserved =
				k_uptime_get();
			*harq_tx_process = &perf_data.client_data.tx_harq_processes[i];
			perf_data.client_data.tx_harq_processes[i].next_new_data_ind =
				!perf_data.client_data.tx_harq_processes[i].next_new_data_ind;

			__ASSERT_NO_MSG(perf_data.client_data.tx_harq_processes[i].process_nbr ==
					i);
			break;
		}
	}

	return return_value;
}

static struct dect_phy_perf_harq_tx_process_info *
dect_phy_perf_harq_tx_process_get_by_process_nbr(uint8_t process_nbr)
{
	__ASSERT_NO_MSG(process_nbr < DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT);

	/* HARQ process nbr is the index of process array */

	return &perf_data.client_data.tx_harq_processes[process_nbr];
}

static uint8_t dect_phy_perf_current_free_harq_tx_process_count_get(void)
{
	uint8_t count = 0;
	int i = 0;

	/* 1st cleanup old ones */
	dect_phy_perf_harq_tx_process_cleanup();

	for (i = 0; i < DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT; i++) {
		if (!perf_data.client_data.tx_harq_processes[i].process_in_use &&
		    perf_data.client_data.tx_harq_processes[i].usable) {
			count++;
		}
	}
	return count;
}

/**************************************************************************************************/

static void dect_phy_perf_mdm_op_complete_cb(
	const struct nrf_modem_dect_phy_op_complete_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_completed_params perf_op_completed_params = {
		.handle = evt->handle,
		.temperature = evt->temp,
		.status = evt->err,
		.time = *time,
	};

	dect_phy_perf_msgq_data_op_add(DECT_PHY_PERF_EVENT_MDM_OP_COMPLETED,
				       (void *)&perf_op_completed_params,
				       sizeof(struct dect_phy_common_op_completed_params));
}

static void dect_phy_perf_mdm_pcc_cb(const struct nrf_modem_dect_phy_pcc_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pcc_rcv_params ctrl_pcc_op_params;
	struct dect_phy_perf_params *cmd_params = &(perf_data.cmd_params);
	uint64_t harq_feedback_start_time = 0;
	struct dect_phy_header_type2_format0_t *header = (void *)&evt->hdr;
	uint16_t len_slots = header->packet_length + 1; /* Always in slots */
	struct dect_phy_header_type2_format1_t feedback_header;

	ctrl_pcc_op_params.pcc_status = *evt;
	ctrl_pcc_op_params.phy_header = evt->hdr;
	ctrl_pcc_op_params.phy_len = header->packet_length;
	ctrl_pcc_op_params.phy_len_type = header->packet_length_type;
	ctrl_pcc_op_params.time = *time;
	ctrl_pcc_op_params.stf_start_time = evt->stf_start_time;

	if (cmd_params->use_harq && cmd_params->role == DECT_PHY_COMMON_ROLE_SERVER) {
		harq_feedback_start_time =
			evt->stf_start_time +
			(len_slots * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
			(cmd_params->server_harq_feedback_tx_delay_subslot_count *
			 DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);
		feedback_header = perf_data.server_data.harq_feedback_data.header;
	}

	/* Provide HARQ feedback if requested */
	perf_data.rx_metrics.rx_last_pcc_received = k_uptime_get();
	if (evt->header_status == NRF_MODEM_DECT_PHY_HDR_STATUS_VALID) {
		struct dect_phy_ctrl_field_common *phy_h = (void *)&evt->hdr;
		const uint8_t *p_ptr = (uint8_t *)&evt->hdr;

		/* Get to transmitter_identity_hi */
		p_ptr++;
		p_ptr++;
		perf_data.rx_metrics.rx_last_tx_id_from_pcc = sys_get_be16(p_ptr);

		perf_data.rx_metrics.rx_last_pcc_phy_pwr = phy_h->transmit_power;
		if (cmd_params->role == DECT_PHY_COMMON_ROLE_SERVER) {
			if (evt->phy_type == DECT_PHY_HEADER_TYPE2) {
				struct nrf_modem_dect_phy_tx_params harq_tx =
					perf_data.server_data.harq_feedback_data.params;

				feedback_header = perf_data.server_data.harq_feedback_data.header;

				__ASSERT_NO_MSG(header->packet_length_type ==
						DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS);
				harq_tx.start_time = harq_feedback_start_time;

				if (header->format == DECT_PHY_HEADER_FORMAT_000 &&
				    header->short_network_id ==
					    perf_data.server_data.harq_feedback_data.header
						    .short_network_id &&
				    header->receiver_identity_hi ==
					    perf_data.server_data.harq_feedback_data.header
						    .transmitter_identity_hi &&
				    header->receiver_identity_lo ==
					    perf_data.server_data.harq_feedback_data.header
						    .transmitter_identity_lo) {
					/* HARQ feedback requested */
					union nrf_modem_dect_phy_hdr phy_header;

					feedback_header.format = DECT_PHY_HEADER_FORMAT_001;
					feedback_header.df_mcs = header->df_mcs;
					feedback_header.transmit_power = header->transmit_power;
					feedback_header.receiver_identity_hi =
						header->transmitter_identity_hi;
					feedback_header.receiver_identity_lo =
						header->transmitter_identity_lo;
					feedback_header.spatial_streams = header->spatial_streams;
					feedback_header.feedback.format1.format = 1;
					feedback_header.feedback.format1.CQI = 1;
					feedback_header.feedback.format1.harq_process_number0 =
						header->df_harq_process_number;

					perf_data.rx_metrics.rx_last_tx_id =
						perf_data.rx_metrics.rx_last_tx_id_from_pcc;

					/* ACK/NACK: According to CRC: */
					feedback_header.feedback.format1.transmission_feedback0 = 0;

					feedback_header.feedback.format1.buffer_status = 0;
					memcpy(&phy_header.type_2, &feedback_header,
					       sizeof(phy_header.type_2));
					harq_tx.phy_header = &phy_header;

					int err = nrf_modem_dect_phy_tx_harq(&harq_tx);

					if (err) {
						printk("nrf_modem_dect_phy_tx_harq() failed: %d\n",
						       err);
					} else {
						perf_data.tx_metrics.tx_total_data_amount +=
							harq_tx.data_size;
					}
				}
			}
		}
	}
	/* With HARQ, schedule RX back right away */
	if (cmd_params->use_harq && cmd_params->role == DECT_PHY_COMMON_ROLE_SERVER) {
		uint64_t start_time = harq_feedback_start_time +
				      (((feedback_header.packet_length + 1) *
					DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
				       (cmd_params->server_harq_feedback_tx_rx_delay_subslot_count *
					DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS));

		dect_phy_perf_server_rx_start(start_time);
	}

	dect_phy_perf_msgq_data_op_add(DECT_PHY_PERF_EVENT_RX_PCC, (void *)&ctrl_pcc_op_params,
				       sizeof(struct dect_phy_common_op_pcc_rcv_params));
}

static void dect_phy_perf_mdm_pcc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pcc_crc_failure_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pcc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_perf_msgq_data_op_add(DECT_PHY_PERF_EVENT_RX_PCC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pcc_crc_fail_params));
}

static void dect_phy_perf_mdm_pdc_cb(const struct nrf_modem_dect_phy_pdc_event *evt, uint64_t *time)
{
	int16_t rssi_level = evt->rssi_2 / 2;

	struct dect_phy_commmon_op_pdc_rcv_params perf_pdc_op_params;

	perf_pdc_op_params.rx_status = *evt;

	perf_pdc_op_params.data_length = evt->len;
	perf_pdc_op_params.time = *time;

	perf_pdc_op_params.rx_pwr_dbm = 0; /* Not needed from PDC */
	perf_pdc_op_params.rx_rssi_level_dbm = rssi_level;
	perf_pdc_op_params.rx_channel = perf_data.cmd_params.channel;

	if (evt->len <= sizeof(perf_pdc_op_params.data)) {
		memcpy(perf_pdc_op_params.data, evt->data, evt->len);
		dect_phy_perf_msgq_data_op_add(DECT_PHY_PERF_EVENT_RX_PDC_DATA,
					       (void *)&perf_pdc_op_params,
					       sizeof(struct dect_phy_commmon_op_pdc_rcv_params));
	} else {
		printk("Received data is too long to be received by PERF TH - discarded (len %d, "
		       "buf size %d)\n",
		       evt->len, sizeof(perf_pdc_op_params.data));
	}
}

static void dect_phy_perf_mdm_pdc_crc_failure_cb(
	const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt, uint64_t *time)
{
	struct dect_phy_common_op_pdc_crc_fail_params pdc_crc_fail_params = {
		.time = *time,
		.crc_failure = *evt,
	};

	dect_phy_perf_msgq_data_op_add(DECT_PHY_PERF_EVENT_RX_PDC_CRC_ERROR,
				       (void *)&pdc_crc_fail_params,
				       sizeof(struct dect_phy_common_op_pdc_crc_fail_params));
}

static void dect_phy_perf_mdm_capability_get_cb(
	const struct nrf_modem_dect_phy_capability_get_event *evt)
{
	for (int i = 0; i < evt->capability->variant_count; i++) {
		if (evt->capability->variant[i].harq_process_count_max !=
		    DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT) {
			printk("Modem HARQ max process count (%d) does not match expected count: "
			       "%d!!!!\n",
			       evt->capability->variant[i].harq_process_count_max,
			       DECT_PHY_PERF_HARQ_TX_PROCESS_COUNT);
		}
	}
}

/**************************************************************************************************/

static void dect_phy_perf_client_tx_with_harq(uint64_t first_possible_tx)
{
	struct dect_phy_perf_params *params = &(perf_data.cmd_params);
	struct dect_phy_perf_harq_tx_process_info *harq_process_data;
	struct nrf_modem_dect_phy_tx_rx_params tx_rx_param;
	uint8_t *encoded_data_to_send = perf_data.client_data.tx_data;
	uint8_t *seq_nbr_ptr = encoded_data_to_send + DECT_PHY_PERF_PDU_HEADER_LEN;
	uint64_t next_tx_time = first_possible_tx;
	uint8_t tx_count = 0;
	uint8_t tx_window;
	struct nrf_modem_dect_phy_tx_params *tx_op_ptr;
	struct nrf_modem_dect_phy_rx_params *rx_op_ptr;
	int ret;

	tx_rx_param.tx = perf_data.client_data.tx_op;
	tx_rx_param.rx = perf_data.client_data.rx_op;

	tx_op_ptr = &tx_rx_param.tx;
	rx_op_ptr = &tx_rx_param.rx;

	tx_window = dect_phy_perf_current_free_harq_tx_process_count_get();
	if (!tx_window) {
		k_timer_start(&harq_tx_window_timer, K_SECONDS(2), K_NO_WAIT);
		if (params->debugs) {
			desh_warn("PERF with HARQ: no TX window");
		}
		return;
	}

	if (tx_op_ptr->handle > DECT_PHY_PERF_TX_HANDLE_END) {
		tx_op_ptr->handle = DECT_PHY_PERF_TX_HANDLE_START;
	}
	if (rx_op_ptr->handle > DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_END) {
		rx_op_ptr->handle = DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_START;
	}

	perf_data.client_data.tx_last_round_starting_handle = tx_op_ptr->handle;
	while (tx_count < tx_window) {
		if (tx_op_ptr->handle > DECT_PHY_PERF_TX_HANDLE_END) {
			tx_op_ptr->handle = DECT_PHY_PERF_TX_HANDLE_START;
		}
		if (rx_op_ptr->handle > DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_END) {
			rx_op_ptr->handle = DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_START;
		}
		if (!dect_phy_perf_harq_tx_process_reserve(&harq_process_data)) {
			desh_error("(%s): cannot reserve HARQ TX process", (__func__));
			break;
		}
		struct dect_phy_header_type2_format0_t *header =
			(struct dect_phy_header_type2_format0_t *)tx_op_ptr->phy_header;

		__ASSERT_NO_MSG(harq_process_data != NULL);

		header->df_redundancy_version = 0;
		header->df_new_data_indication_toggle = harq_process_data->next_new_data_ind;
		header->df_harq_process_number = harq_process_data->process_nbr;
		header->feedback.format1.format = 0; /* No feedback */
		harq_process_data->seq_nbr = perf_data.client_data.tx_last_seq_nbr;
		tx_op_ptr->start_time = next_tx_time;

		/* RX time is relative from TX end */
		rx_op_ptr->start_time = params->client_harq_feedback_rx_delay_subslot_count *
					DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;

		/* Do actual TX and start RX for HARQ feedback after "HARQ feedback delay" */
		ret = nrf_modem_dect_phy_tx_rx(&tx_rx_param);
		if (ret) {
			desh_error("(%s): nrf_modem_dect_phy_tx_rx failed %d (handle %d)\n",
				   (__func__), ret, tx_op_ptr->handle);
			break;
		}
		perf_data.client_data.tx_last_scheduled_mdm_op_start_time_mdm_ticks = next_tx_time;

		perf_data.tx_metrics.tx_total_data_amount += tx_op_ptr->data_size;
		perf_data.tx_metrics.tx_total_pkt_count++;

		/* Modify seq nbr for next round */
		perf_data.client_data.tx_last_seq_nbr++;
		tx_count++;
		dect_common_utils_16bit_be_write(seq_nbr_ptr,
						 perf_data.client_data.tx_last_seq_nbr);
		tx_op_ptr->handle++;
		rx_op_ptr->handle++;

		/* One TX with HARQ iteration:
		 * actual TX + HARQ feedback delay + HARQ feedback RX + gap
		 */
		next_tx_time +=
			((params->slot_count * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
			 (params->client_harq_feedback_rx_delay_subslot_count *
			  DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS) +
			 rx_op_ptr->duration + perf_data.cmd_params.slot_gap_count_in_mdm_ticks);
	}
	perf_data.client_data.tx_harq_feedback_rx_last_round_middle_handle =
		(perf_data.client_data.tx_op.handle + (tx_op_ptr->handle - 1)) / 2;
	perf_data.client_data.tx_op.handle = tx_op_ptr->handle;
	perf_data.client_data.rx_op.handle = rx_op_ptr->handle;
}

static void dect_phy_perf_client_tx_no_harq(uint64_t first_possible_tx)
{
	uint8_t *encoded_data_to_send = perf_data.client_data.tx_data;
	uint8_t *seq_nbr_ptr = encoded_data_to_send + DECT_PHY_PERF_PDU_HEADER_LEN;
	uint64_t next_tx_time = first_possible_tx;
	uint8_t tx_count = 0;
	uint8_t tx_window = DECT_PHY_PERF_CLIENT_TX_WINDOW_COUNT;
	int ret;

	if (perf_data.client_data.tx_op.handle > DECT_PHY_PERF_TX_HANDLE_END) {
		perf_data.client_data.tx_op.handle = DECT_PHY_PERF_TX_HANDLE_START;
	}

	perf_data.client_data.tx_last_round_starting_handle = perf_data.client_data.tx_op.handle;
	while (tx_count < tx_window) {
		if (perf_data.client_data.tx_op.handle > DECT_PHY_PERF_TX_HANDLE_END) {
			perf_data.client_data.tx_op.handle = DECT_PHY_PERF_TX_HANDLE_START;
		}

		perf_data.client_data.tx_op.start_time = next_tx_time;
		ret = nrf_modem_dect_phy_tx(&perf_data.client_data.tx_op);
		if (ret) {
			desh_error("(%s): nrf_modem_dect_phy_transmit failed %d (handle %d)\n",
				   (__func__), ret, perf_data.client_data.tx_op.handle);
			break;
		}
		perf_data.client_data.tx_last_scheduled_mdm_op_start_time_mdm_ticks = next_tx_time;

		perf_data.tx_metrics.tx_total_data_amount += perf_data.client_data.tx_op.data_size;
		perf_data.tx_metrics.tx_total_pkt_count++;

		/* Modify seq nbr for next round */
		perf_data.client_data.tx_last_seq_nbr++;
		tx_count++;
		dect_common_utils_16bit_be_write(seq_nbr_ptr,
						 perf_data.client_data.tx_last_seq_nbr);
		perf_data.client_data.tx_op.handle++;
		next_tx_time += ((perf_data.cmd_params.slot_count *
				  DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
				 perf_data.cmd_params.slot_gap_count_in_mdm_ticks);
	}
}

static void dect_phy_perf_client_tx(uint64_t first_possible_tx)
{
	struct dect_phy_perf_params *params = &(perf_data.cmd_params);

	if (params->use_harq) {
		dect_phy_perf_client_tx_with_harq(first_possible_tx);
	} else {
		dect_phy_perf_client_tx_no_harq(first_possible_tx);
	}
}

static void dect_phy_perf_cmd_done(void)
{
	if (!perf_data.perf_ongoing) {
		desh_error("%s called when perf cmd is not ongoing - caller %pS",
			(__func__),
			__builtin_return_address(0));
		return;
	}
	perf_data.perf_ongoing = false;

	/* Mdm phy api deinit is done by dect_phy_ctrl */
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_PERF_CMD_DONE);
}

static int dect_phy_perf_client_start(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_perf_params *params = &(perf_data.cmd_params);

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
		.df_new_data_indication_toggle = 1, /* to be toggled later */
		.df_redundancy_version = 0,  /* 1st tx */
		.spatial_streams = 2,
		.feedback = {
				.format1 = {
						.format = 0,
						.CQI = 1, /* No meaning in our case  */
						.harq_process_number0 = 0,
						.transmission_feedback0 = 1,
						.buffer_status = 0, /* No meaning in our case  */
					},
			},
	};
	dect_phy_perf_pdu_t perf_pdu;
	uint8_t *encoded_data_to_send = perf_data.client_data.tx_data;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx =
		time_now +
		(US_TO_MODEM_TICKS(2 * current_settings->scheduler.scheduling_delay_us + 4000));

	/* Sending max amount of data that can be encoded to given slot amount */
	int16_t perf_pdu_byte_count =
		dect_common_utils_slots_in_bytes(params->slot_count - 1, params->tx_mcs);

	if (perf_pdu_byte_count <= 0) {
		desh_error("Unsupported slot/mcs combination");
		return -1;
	}

	header.format = DECT_PHY_HEADER_FORMAT_001;

	if (params->use_harq) {
		header.format = DECT_PHY_HEADER_FORMAT_000;
		header.feedback.format1.format = 1;
		header.feedback.format1.harq_process_number0 = 0; /* Starting process # */
		header.df_new_data_indication_toggle = true;
		first_possible_tx += MS_TO_MODEM_TICKS(DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS);
	}

	desh_print(
		"Starting perf client on channel %d: byte count per TX: %d, slots %d, gap %lld mdm "
		"ticks, mcs %d, duration %d secs, expected RSSI level on RX %d.",
		params->channel, perf_pdu_byte_count, params->slot_count,
		params->slot_gap_count_in_mdm_ticks, params->tx_mcs, params->duration_secs,
		params->expected_rx_rssi_level);

	uint16_t perf_pdu_payload_byte_count =
		perf_pdu_byte_count - DECT_PHY_PERF_TX_DATA_PDU_LEN_WITHOUT_PAYLOAD;

	/* Encode perf pdu */
	memcpy(&perf_data.client_data.tx_phy_header.type_2, &header,
	       sizeof(perf_data.client_data.tx_phy_header.type_2));

	perf_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PERF_TX_DATA;
	perf_pdu.header.transmitter_id = current_settings->common.transmitter_id;
	perf_pdu.message.tx_data.seq_nbr = 1;
	perf_pdu.message.tx_data.payload_length = perf_pdu_payload_byte_count;
	dect_common_utils_fill_with_repeating_pattern(perf_pdu.message.tx_data.pdu_payload,
						      perf_pdu_payload_byte_count);
	dect_phy_perf_pdu_encode(encoded_data_to_send, &perf_pdu);

	perf_data.time_started_ms = k_uptime_get();

	memset(&perf_data.tx_metrics, 0, sizeof(struct dect_phy_perf_tx_metrics));

	/* Fill TX operation */
	perf_data.client_data.tx_last_seq_nbr = perf_pdu.message.tx_data.seq_nbr;
	perf_data.client_data.tx_data_len = perf_pdu_byte_count;

	perf_data.client_data.tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	perf_data.client_data.tx_op.carrier = params->channel;
	perf_data.client_data.tx_op.data_size = perf_pdu_byte_count;
	perf_data.client_data.tx_op.data = encoded_data_to_send;
	perf_data.client_data.tx_op.lbt_period = 0;
	perf_data.client_data.tx_op.network_id = current_settings->common.network_id;
	perf_data.client_data.tx_op.phy_header = &perf_data.client_data.tx_phy_header;
	perf_data.client_data.tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
	perf_data.client_data.tx_op.handle = DECT_PHY_PERF_TX_HANDLE_START;

	/* Only used with HARQ for receiving HARQ feedback */
	perf_data.client_data.rx_op.handle = DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_START;
	perf_data.client_data.rx_op.network_id = current_settings->common.network_id;
	perf_data.client_data.rx_op.filter.is_short_network_id_used = true;
	perf_data.client_data.rx_op.filter.short_network_id = current_settings->common.network_id;
	perf_data.client_data.rx_op.filter.receiver_identity =
		current_settings->common.transmitter_id;
	perf_data.client_data.rx_op.carrier = params->channel;
	perf_data.client_data.rx_op.duration = params->client_harq_feedback_rx_subslot_count *
					       DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;
	perf_data.client_data.rx_op.mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT;
	perf_data.client_data.rx_op.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED;
	perf_data.client_data.rx_op.rssi_level = params->expected_rx_rssi_level;

	dect_phy_perf_client_tx(first_possible_tx);

	return 0;
}

enum dect_phy_perf_server_restart_mode {
	START = 0,
	RESTART,
	RESTART_AFTER_RESULT_SENDING,
};

static int dect_phy_perf_start(struct dect_phy_perf_params *params,
			       enum dect_phy_perf_server_restart_mode restart_mode)
{
	int ret = 0;

	if (restart_mode == RESTART) {
		if (perf_data.rx_metrics.rx_last_tx_id) {
			perf_data.rx_metrics.rx_restarted_count++;
		}
	} else {
		memset(&perf_data, 0, sizeof(struct dect_phy_perf_data));
		perf_data.time_started_ms = k_uptime_get();
		dect_phy_perf_harq_tx_process_table_init(params->client_harq_process_nbr_max);
		dect_phy_perf_data_rx_metrics_init();
	}

	perf_data.perf_ongoing = true;
	perf_data.cmd_params = *params;
	if (params->role == DECT_PHY_COMMON_ROLE_CLIENT) {
		ret = dect_phy_perf_client_start();
	} else {
		__ASSERT_NO_MSG(params->role == DECT_PHY_COMMON_ROLE_SERVER);
		enum nrf_modem_dect_phy_radio_mode radio_mode;
		uint64_t next_possible_start_time = 0;

		ret = dect_phy_ctrl_current_radio_mode_get(&radio_mode);

		desh_print("Starting server RX: channel %d, exp_rssi_level %d, duration %d secs.",
			   params->channel, params->expected_rx_rssi_level, params->duration_secs);

		if (!ret && radio_mode != NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY) {
			uint64_t time_now = dect_app_modem_time_now();
			struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

			next_possible_start_time =
				time_now + (3 * US_TO_MODEM_TICKS(
					current_settings->scheduler.scheduling_delay_us));
		}
		ret = dect_phy_perf_server_rx_start(next_possible_start_time);
	}
	return ret;
}

/**************************************************************************************************/

static int16_t dect_phy_perf_time_secs_left(void)
{
	int64_t time_orig_started = perf_data.time_started_ms;
	int64_t running_time_ms = k_uptime_delta(&time_orig_started);
	uint16_t running_time_secs = running_time_ms / 1000;
	struct dect_phy_perf_params *params = &(perf_data.cmd_params);
	int16_t return_value = 0;

	if (params->duration_secs == DECT_PHY_PERF_DURATION_INFINITE) {
		/* Forever */
		return_value = params->duration_secs;
	} else if (params->duration_secs > running_time_secs) {
		/* Continue as server */
		return_value = params->duration_secs - running_time_secs;
	} else {
		return_value = 0;
	}
	return return_value;
}

static int dect_phy_perf_tx_request_results(void)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_perf_params *params = &(perf_data.cmd_params);

	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 0,
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001,
		.short_network_id = (uint8_t)(current_settings->common.network_id & 0xFF),
		.transmitter_identity_hi = (uint8_t)(current_settings->common.transmitter_id >> 8),
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
	dect_phy_perf_pdu_t perf_pdu;
	uint8_t encoded_data_to_send[64];
	int ret = 0;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx = time_now + SECONDS_TO_MODEM_TICKS(2);

	memcpy(&phy_header.type_2, &header, sizeof(phy_header.type_2));

	perf_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_REQ;
	perf_pdu.message.results_req.unused = 0;
	perf_pdu.header.transmitter_id = current_settings->common.transmitter_id;
	dect_phy_perf_pdu_encode(encoded_data_to_send, &perf_pdu);

	tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	tx_op.carrier = params->channel;
	tx_op.data_size = DECT_PHY_PERF_RESULTS_REQ_LEN;
	tx_op.data = encoded_data_to_send;
	tx_op.lbt_period = 0;
	tx_op.network_id = current_settings->common.network_id;
	tx_op.phy_header = &phy_header;
	tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
	tx_op.handle = DECT_PHY_PERF_RESULTS_REQ_TX_HANDLE;
	tx_op.start_time = first_possible_tx;

	ret = nrf_modem_dect_phy_tx(&tx_op);
	if (ret) {
		desh_error("(%s): nrf_modem_dect_phy_tx failed %d (handle %d)\n", (__func__), ret,
			   tx_op.handle);
	} else {
		/* RX for the results response */
		struct nrf_modem_dect_phy_rx_params rx_op = {
			.handle = DECT_PHY_PERF_RESULTS_RESP_RX_HANDLE,
			.start_time =
				first_possible_tx + (3 * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS),
			.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
			.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
			.carrier = params->channel,
			.network_id = current_settings->common.network_id,
			.rssi_level = params->expected_rx_rssi_level,
			.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
		};

		rx_op.filter.is_short_network_id_used = true;
		rx_op.filter.short_network_id = current_settings->common.network_id;
		rx_op.filter.receiver_identity = current_settings->common.transmitter_id;
		rx_op.duration = SECONDS_TO_MODEM_TICKS(5);

		ret = dect_phy_common_rx_op(&rx_op);
		if (ret) {
			desh_error("nrf_modem_dect_phy_rx failed: ret %d, handle", ret,
				   rx_op.handle);
		}
	}

	return ret;
}

static int
dect_phy_perf_client_report_local_results_and_req_srv_results(int64_t *elapsed_time_ms_ptr)
{
	if (perf_data.client_data.tx_results_from_server_requested) {
		return 0;
	}

	int64_t time_orig_started = perf_data.time_started_ms;
	int64_t elapsed_time_ms = k_uptime_delta(&time_orig_started);
	int ret = 0;

	if (elapsed_time_ms_ptr != NULL) {
		elapsed_time_ms = *elapsed_time_ms_ptr;
	}

	double perf = dect_phy_perf_calculate_throughput(perf_data.tx_metrics.tx_total_data_amount,
							 elapsed_time_ms);

	desh_print("perf tx operation completed:");
	desh_print("  total amount of data sent: %d bytes",
		   perf_data.tx_metrics.tx_total_data_amount);
	desh_print("  packet count:              %d", perf_data.tx_metrics.tx_total_pkt_count);
	desh_print("  elapsed time:              %.2f seconds", (double)elapsed_time_ms / 1000);
	desh_print("  data rates:                %.2f kbits/seconds", (perf / 1000));

	if (perf_data.cmd_params.use_harq) {
		desh_print("  No HARQ feedback count:                  %d",
			   perf_data.tx_metrics.tx_harq_timeout_count);
		desh_print("  HARQ ACK receive count:                  %d",
			   perf_data.rx_metrics.harq_ack_rx_count);
		desh_print("  HARQ NACK receive count:                 %d",
			   perf_data.rx_metrics.harq_nack_rx_count);
		desh_print("  HARQ reset NACK receive count:           %d",
			   perf_data.rx_metrics.harq_reset_nack_rx_count);
		desh_print("  PCC CRC error count (HARQ feedback RX):  %d",
			   perf_data.rx_metrics.rx_pcc_crc_error_count);
		desh_print("  PDC CRC error count (HARQ feedback RX):  %d",
			   perf_data.rx_metrics.rx_pdc_crc_error_count);
		desh_print("  RX: RSSI: min %d, max %d", perf_data.rx_metrics.rx_rssi_low_level,
			   perf_data.rx_metrics.rx_rssi_high_level);
		desh_print("  RX: SNR: min %d, max %d", perf_data.rx_metrics.rx_snr_low,
			   perf_data.rx_metrics.rx_snr_high);
		desh_print("  RX: pwr: min %d, max %d",
			   perf_data.rx_metrics.rx_phy_transmit_pwr_low,
			   perf_data.rx_metrics.rx_phy_transmit_pwr_high);
	}

	ret = dect_phy_perf_tx_request_results();
	if (ret) {
		desh_error("Failed (err =%d) to request results from the server. Perf cmd done.",
			   ret);
		dect_phy_perf_cmd_done();
	} else {
		perf_data.client_data.tx_results_from_server_requested = true;
		desh_print("Requested results from the server...");
	}
	return ret;
}

static int dect_phy_perf_server_results_tx(char *result_str)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_perf_params *params = &(perf_data.cmd_params);
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
			perf_data.rx_metrics.rx_last_pcc_phy_pwr, params->channel),
		.receiver_identity_hi = (uint8_t)(perf_data.rx_metrics.rx_last_tx_id >> 8),
		.receiver_identity_lo = (uint8_t)(perf_data.rx_metrics.rx_last_tx_id & 0xFF),
		.feedback.format1.format = 0,
	};
	union nrf_modem_dect_phy_hdr phy_header;
	struct nrf_modem_dect_phy_tx_params tx_op;
	dect_phy_perf_pdu_t perf_pdu;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN];
	int ret = 0;

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t first_possible_tx = time_now + SECONDS_TO_MODEM_TICKS(2);

	uint16_t bytes_to_send = DECT_PHY_PERF_PDU_COMMON_PART_LEN + (strlen(result_str) + 1);

	perf_pdu.header.message_type = DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_RESP;
	perf_pdu.header.transmitter_id = current_settings->common.transmitter_id;

	/* Calculate the slot count needs */
	header.packet_length = dect_common_utils_phy_packet_length_calculate(
		bytes_to_send, header.packet_length_type, header.df_mcs);
	if (header.packet_length < 0) {
		desh_error("(%s): Phy pkt len calculation failed", (__func__));
		return -EINVAL;
	}
	memcpy(&phy_header.type_2, &header, sizeof(phy_header.type_2));

	strcpy(perf_pdu.message.results.results_str, result_str);
	dect_phy_perf_pdu_encode(encoded_data_to_send, &perf_pdu);

	tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
	tx_op.carrier = params->channel;
	tx_op.data_size = bytes_to_send;
	tx_op.data = encoded_data_to_send;
	tx_op.lbt_period = 0;
	tx_op.network_id = current_settings->common.network_id;
	tx_op.phy_header = &phy_header;
	tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
	tx_op.handle = DECT_PHY_PERF_RESULTS_RESP_TX_HANDLE;
	tx_op.start_time = first_possible_tx;

	perf_data.server_data.rx_results_sent = true;

	ret = nrf_modem_dect_phy_tx(&tx_op);
	if (ret) {
		perf_data.server_data.rx_results_sent = false;
		desh_error("(%s): nrf_modem_dect_phy_tx failed %d (handle %d)\n", (__func__), ret,
			   tx_op.handle);
	} else {
		desh_print("Server results TX scheduled: %d bytes (%d slots, MCS-%d).",
			   bytes_to_send, header.packet_length + 1, header.df_mcs);
	}

	return ret;
}

static void dect_phy_perf_server_report_local_and_tx_results(void)
{

	if (!perf_data.rx_metrics.rx_total_data_amount) {
		desh_warn("No perf data received on server side.");
	}

	int64_t elapsed_time_ms = perf_data.rx_metrics.rx_last_data_received -
				  perf_data.rx_metrics.rx_1st_data_received;
	double perf = dect_phy_perf_calculate_throughput(perf_data.rx_metrics.rx_total_data_amount,
							 elapsed_time_ms);
	char results_str[DECT_PHY_PERF_RESULTS_DATA_MAX_LEN];

	memset(results_str, 0, sizeof(results_str));

	sprintf(results_str, "perf server rx operation from tx id %d:\n",
		perf_data.rx_metrics.rx_last_tx_id);
	sprintf(results_str + strlen(results_str), "  total amount of data:   %d bytes\n",
		perf_data.rx_metrics.rx_total_data_amount);
	sprintf(results_str + strlen(results_str), "  packet count:           %d\n",
		perf_data.rx_metrics.rx_total_pkt_count);
	sprintf(results_str + strlen(results_str), "  elapsed time:           %.2f secs\n",
		(double)elapsed_time_ms / 1000);
	sprintf(results_str + strlen(results_str), "  data rates:             %.2f kbits/secs\n",
		(perf / 1000));
	sprintf(results_str + strlen(results_str), "  rx restarted count:     %d\n",
		perf_data.rx_metrics.rx_restarted_count);
	if (perf_data.tx_metrics.tx_total_data_amount) {
		sprintf(results_str + strlen(results_str), "  HARQ feedback tx bytes: %d\n",
			perf_data.tx_metrics.tx_total_data_amount);
	}
	sprintf(results_str + strlen(results_str), "  PCC CRC errors: %d\n",
		perf_data.rx_metrics.rx_pcc_crc_error_count);
	sprintf(results_str + strlen(results_str), "  PDC CRC errors: %d\n",
		perf_data.rx_metrics.rx_pdc_crc_error_count);
	sprintf(results_str + strlen(results_str), "  out of seqs:    %d\n",
		perf_data.rx_metrics.rx_out_of_seq_count);
	sprintf(results_str + strlen(results_str), "  PCC RX: MCS:    %d\n",
		perf_data.rx_metrics.rx_testing_mcs);
	sprintf(results_str + strlen(results_str), "  PCC RX: RSSI: min:  %d, max: %ddBm\n",
		perf_data.rx_metrics.rx_rssi_low_level, perf_data.rx_metrics.rx_rssi_high_level);
	sprintf(results_str + strlen(results_str), "  PCC RX: TX pwr: min %d, max %d dBm\n",
		dect_common_utils_phy_tx_power_to_dbm(perf_data.rx_metrics.rx_phy_transmit_pwr_low),
		dect_common_utils_phy_tx_power_to_dbm(
			perf_data.rx_metrics.rx_phy_transmit_pwr_high));
	sprintf(results_str + strlen(results_str), "  PCC RX: SNR: min:    %d, max: %d\n",
		perf_data.rx_metrics.rx_snr_low, perf_data.rx_metrics.rx_snr_high);

	desh_print("server: sending result response of total length: %d:\n\"%s\"\n",
		   (strlen(results_str) + 1), results_str);

	/* Send results to client: */
	(void)nrf_modem_dect_phy_cancel(DECT_PHY_PERF_SERVER_RX_HANDLE);
	if (dect_phy_perf_server_results_tx(results_str)) {
		desh_error("Cannot start sending server results");
	}

	/* Clear results */
	dect_phy_perf_data_rx_metrics_init();
}

void dect_phy_perf_mdm_op_completed(
	struct dect_phy_common_op_completed_params *mdm_completed_params)
{
	if (perf_data.perf_ongoing) {
		if (DECT_PHY_PERF_TX_HANDLE_IN_RANGE(mdm_completed_params->handle)) {
			int64_t time_orig_started = perf_data.time_started_ms;
			int64_t elapsed_time_ms = k_uptime_delta(&time_orig_started);

			if (!perf_data.perf_ongoing ||
			    (elapsed_time_ms >= (perf_data.cmd_params.duration_secs * 1000))) {
				(void)dect_phy_perf_client_report_local_results_and_req_srv_results(
					&elapsed_time_ms);
				return;
			}
			struct dect_phy_perf_params *params = &(perf_data.cmd_params);
			uint32_t harq_mdm_ticks = 0;

			if (params->use_harq) {
				harq_mdm_ticks =
					(params->client_harq_feedback_rx_delay_subslot_count *
					 DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS) +
					(params->client_harq_feedback_rx_subslot_count *
					 DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);
			}

			if (mdm_completed_params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				/* TX was not done */
				dect_phy_perf_tx_total_data_decrease();
				perf_data.tx_metrics.tx_total_pkt_count--;
			}

			if (mdm_completed_params->handle ==
				    perf_data.client_data.tx_last_round_starting_handle ||
			    mdm_completed_params->handle ==
				    perf_data.client_data
					    .tx_harq_feedback_rx_last_round_middle_handle) {
				struct dect_phy_settings *current_settings =
					dect_common_settings_ref_get();
				uint64_t time_now = dect_app_modem_time_now();
				uint64_t first_possible_tx =
					time_now +
					(US_TO_MODEM_TICKS(
						current_settings->scheduler.scheduling_delay_us));

				/* One TX with HARQ iteration:
				 * actual TX + HARQ feedback delay + HARQ feedback RX + gap
				 */
				uint64_t iteration_mdm_ticks =
					harq_mdm_ticks +
					((params->slot_count *
					  DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
					 params->slot_gap_count_in_mdm_ticks);
				uint64_t next_scheduled_tx =
					perf_data.client_data
						.tx_last_scheduled_mdm_op_start_time_mdm_ticks +
					iteration_mdm_ticks;
				uint64_t next_possible_tx = next_scheduled_tx;

				while (next_possible_tx < first_possible_tx) {
					next_possible_tx = next_possible_tx + iteration_mdm_ticks;
				}
				dect_phy_perf_client_tx(next_possible_tx);
			}

		} else if (mdm_completed_params->handle == DECT_PHY_PERF_SERVER_RX_HANDLE) {
			/* Restart server in case of when max duration of RX operation elapses */
			uint16_t secs_left = dect_phy_perf_time_secs_left();
			int64_t z_ticks_rx_scheduled = perf_data.rx_metrics.rx_enabled_z_ticks;
			int64_t z_delta_ms = k_uptime_delta(&z_ticks_rx_scheduled);

			dect_phy_perf_server_data_ongoing_rx_count_decrease();

			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				if (secs_left == 0) {
					dect_phy_perf_msgq_non_data_op_add(
						DECT_PHY_PERF_EVENT_SERVER_REPORT);
					desh_print("Server RX completed.");
					dect_phy_perf_cmd_done();
				} else if (!perf_data.server_data.rx_results_sent) {
					if (perf_data.cmd_params.role ==
						    DECT_PHY_COMMON_ROLE_SERVER &&
					    (perf_data.rx_metrics.rx_req_count_on_going == 0 ||
					     z_delta_ms > 62000)) {
						struct dect_phy_perf_params copy_params =
							perf_data.cmd_params;

						copy_params.duration_secs = secs_left;
						desh_print("Server RX completed but duration left: "
							   "restarting (%d secs).",
							   secs_left);
						dect_phy_perf_start(&copy_params, RESTART);
					}
				}
			} else {
				/* Let's restart only if failure is not happening very fast which
				 * usually means a fundamental failure and then it is better to
				 * quit.
				 */
				if (perf_data.cmd_params.role == DECT_PHY_COMMON_ROLE_SERVER &&
				    perf_data.rx_metrics.rx_req_count_on_going == 0 &&
				    z_delta_ms > 500) {
					struct dect_phy_perf_params copy_params =
						perf_data.cmd_params;

					copy_params.duration_secs = secs_left;
					dect_phy_perf_start(&copy_params, RESTART);
				} else {
					desh_error(
						"perf server RX failed - perf server RX stopped.");
					dect_phy_perf_cmd_done();
				}
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PERF_RESULTS_REQ_TX_HANDLE) {
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_print("RESULT_REQ sent.");
			} else {
				desh_warn("RESULT_REQ sending failed.");
			}
		} else if (mdm_completed_params->handle == DECT_PHY_PERF_RESULTS_RESP_TX_HANDLE) {
			perf_data.server_data.rx_results_sent = false;
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_print("RESULT_RESP sent.");
			} else {
				desh_warn("RESULT_RESP sending failed.");
			}

			uint16_t secs_left = dect_phy_perf_time_secs_left();

			if (secs_left) {
				/* Continue as server */
				if (secs_left == DECT_PHY_PERF_DURATION_INFINITE) {
					desh_print("Server RX will be continued after result TX is "
						   "done.");
				} else {
					desh_print("Server RX will be continued (%d secs) after "
						   "result TX is done.",
						   secs_left);
				}
				if (perf_data.cmd_params.duration_secs) {
					struct dect_phy_perf_params copy_params =
						perf_data.cmd_params;

					copy_params.duration_secs = secs_left;
					dect_phy_perf_start(&copy_params,
							    RESTART_AFTER_RESULT_SENDING);
				}
			} else {
				dect_phy_perf_cmd_done();
			}

		} else if (mdm_completed_params->handle == DECT_PHY_PERF_RESULTS_RESP_RX_HANDLE) {
			/* Client */
			if (mdm_completed_params->status == NRF_MODEM_DECT_PHY_SUCCESS) {
				desh_print("RX for perf results done.");
			} else {
				desh_print("RX for perf results failed.");
			}
			dect_phy_perf_cmd_done();
		}
	}
}

/**************************************************************************************************/

K_MSGQ_DEFINE(dect_phy_perf_op_event_msgq, sizeof(struct dect_phy_common_op_event_msgq_item), 10,
	      4);

/**************************************************************************************************/

#define DECT_PHY_PERF_THREAD_STACK_SIZE 4096
#define DECT_PHY_PERF_THREAD_PRIORITY	5

static void dect_phy_perf_thread_fn(void)
{
	struct dect_phy_common_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_perf_op_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case DECT_PHY_PERF_EVENT_CMD_DONE: {
			dect_phy_perf_cmd_done();
			break;
		}
		case DECT_PHY_PERF_EVENT_HARQ_TX_WIN_TIMER: {
			if (!perf_data.perf_ongoing) {
				break;
			}
			if (!dect_phy_perf_time_secs_left()) {
				desh_print("HARQ TX window was full - timeoutting.");
				dect_phy_perf_cmd_stop();
				break;
			}

			struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
			struct dect_phy_perf_params *params = &(perf_data.cmd_params);
			uint64_t time_now = dect_app_modem_time_now();
			uint32_t harq_mdm_ticks =
				(params->client_harq_feedback_rx_delay_subslot_count *
				 DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS) +
				(params->client_harq_feedback_rx_subslot_count *
				 DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);
			uint64_t first_possible_tx =
				time_now +
				(US_TO_MODEM_TICKS(
					current_settings->scheduler.scheduling_delay_us));
			/* One TX with HARQ iteration:
			 * actual TX + HARQ feedback delay + RX for HARQ feedback + gap
			 */
			uint64_t iteration_mdm_ticks =
				harq_mdm_ticks +
				((params->slot_count * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
				 params->slot_gap_count_in_mdm_ticks);
			uint64_t next_scheduled_tx =
				perf_data.client_data
					.tx_last_scheduled_mdm_op_start_time_mdm_ticks +
				iteration_mdm_ticks;
			uint64_t next_possible_tx = next_scheduled_tx;

			while (next_possible_tx < first_possible_tx) {
				next_possible_tx = next_possible_tx + iteration_mdm_ticks;
			}
			dect_phy_perf_client_tx_with_harq(next_possible_tx);
			break;
		}

		case DECT_PHY_PERF_EVENT_MDM_OP_COMPLETED: {
			struct dect_phy_common_op_completed_params *params =
				(struct dect_phy_common_op_completed_params *)event.data;

			if (params->status != NRF_MODEM_DECT_PHY_SUCCESS) {
				char tmp_str[128] = {0};

				dect_common_utils_modem_phy_err_to_string(
					params->status, params->temperature, tmp_str);

				if (DECT_PHY_PERF_TX_HANDLE_IN_RANGE(params->handle) ||
				    params->handle == DECT_PHY_PERF_SERVER_RX_HANDLE ||
				    params->handle == DECT_PHY_PERF_RESULTS_REQ_TX_HANDLE ||
				    params->handle == DECT_PHY_PERF_RESULTS_RESP_TX_HANDLE ||
				    params->handle == DECT_PHY_PERF_RESULTS_RESP_RX_HANDLE) {
					desh_warn("%s: perf modem operation failed: %s, "
						  "handle %d",
						  __func__, tmp_str, params->handle);
					dect_phy_perf_mdm_op_completed(params);
				} else if (params->handle == DECT_HARQ_FEEDBACK_TX_HANDLE) {
					desh_error("%s: cannot TX HARQ feedback: %s", __func__,
						   tmp_str);
				} else if (params->handle ==
					   DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_IN_RANGE(
						   params->handle)) {
					desh_error("%s: cannot RX for HARQ feedback: %s", __func__,
						   tmp_str);
				} else {
					desh_error(
						"%s: operation (handle %d) failed with status: %s",
						__func__, params->handle, tmp_str);
				}
			} else {
				if (DECT_PHY_PERF_TX_HANDLE_IN_RANGE(params->handle) ||
				    params->handle == DECT_PHY_PERF_SERVER_RX_HANDLE ||
				    params->handle == DECT_PHY_PERF_RESULTS_REQ_TX_HANDLE ||
				    params->handle == DECT_PHY_PERF_RESULTS_RESP_TX_HANDLE ||
				    params->handle == DECT_PHY_PERF_RESULTS_RESP_RX_HANDLE) {
					dect_phy_perf_mdm_op_completed(params);
				}
			}
			break;
		}
		case DECT_PHY_PERF_EVENT_SERVER_REPORT: {
			dect_phy_perf_server_report_local_and_tx_results();
			break;
		}
		case DECT_PHY_PERF_EVENT_RX_PCC_CRC_ERROR: {
			struct dect_phy_common_op_pcc_crc_fail_params *params =
				(struct dect_phy_common_op_pcc_crc_fail_params *)event.data;
			struct dect_phy_perf_params *cmd_params = &(perf_data.cmd_params);

			if (perf_data.perf_ongoing) {
				dect_phy_perf_rx_on_pcc_crc_failure();
			}

			if (cmd_params->debugs) {
				desh_warn("PERF: RX PCC CRC error (time %llu): SNR %d, RSSI-2 %d "
					  "(%d dBm)",
					  params->time, params->crc_failure.snr,
					  params->crc_failure.rssi_2,
					  (params->crc_failure.rssi_2 / 2));
			}
			break;
		}
		case DECT_PHY_PERF_EVENT_RX_PDC_CRC_ERROR: {
			struct dect_phy_common_op_pdc_crc_fail_params *params =
				(struct dect_phy_common_op_pdc_crc_fail_params *)event.data;
			struct dect_phy_perf_params *cmd_params = &(perf_data.cmd_params);

			if (perf_data.perf_ongoing) {
				dect_phy_perf_rx_on_pdc_crc_failure();
			}
			if (cmd_params->debugs) {
				desh_warn("PERF: RX PDC CRC error (time %llu): SNR %d, RSSI-2 %d "
					  "(%d dBm)",
					  params->time, params->crc_failure.snr,
					  params->crc_failure.rssi_2,
					  (params->crc_failure.rssi_2 / 2));
			}
			break;
		}
		case DECT_PHY_PERF_EVENT_RX_PCC: {
			struct dect_phy_common_op_pcc_rcv_params *params =
				(struct dect_phy_common_op_pcc_rcv_params *)event.data;
			struct dect_phy_perf_params *cmd_params = &(perf_data.cmd_params);
			int16_t rssi_level = params->pcc_status.rssi_2 / 2;
			struct dect_phy_ctrl_field_common *phy_h = (void *)&(params->phy_header);

			perf_data.rx_metrics.rx_latest_mcs = phy_h->df_mcs;

			/* Store min/max RSSI/SNR/tx pwr as seen on PCC RX */
			if (rssi_level > perf_data.rx_metrics.rx_rssi_high_level) {
				perf_data.rx_metrics.rx_rssi_high_level = rssi_level;
			}
			if (rssi_level < perf_data.rx_metrics.rx_rssi_low_level) {
				perf_data.rx_metrics.rx_rssi_low_level = rssi_level;
			}
			if (phy_h->transmit_power > perf_data.rx_metrics.rx_phy_transmit_pwr_high) {
				perf_data.rx_metrics.rx_phy_transmit_pwr_high =
					phy_h->transmit_power;
			}
			if (phy_h->transmit_power < perf_data.rx_metrics.rx_phy_transmit_pwr_low) {
				perf_data.rx_metrics.rx_phy_transmit_pwr_low =
					phy_h->transmit_power;
			}

			if (params->pcc_status.snr > perf_data.rx_metrics.rx_snr_high) {
				perf_data.rx_metrics.rx_snr_high = params->pcc_status.snr;
			}
			if (params->pcc_status.snr < perf_data.rx_metrics.rx_snr_low) {
				perf_data.rx_metrics.rx_snr_low = params->pcc_status.snr;
			}

			if (cmd_params->use_harq &&
			    cmd_params->role == DECT_PHY_COMMON_ROLE_CLIENT &&
			    params->pcc_status.header_status ==
				    NRF_MODEM_DECT_PHY_HDR_STATUS_VALID &&
			    params->pcc_status.phy_type == DECT_PHY_HEADER_TYPE2) {

				/* Handle HARQ feedback response */
				struct dect_phy_header_type2_format1_t *header =
					(void *)&(params->phy_header);
				uint8_t rcv_harq_process_nbr =
					header->feedback.format1.harq_process_number0;
				struct dect_phy_perf_harq_tx_process_info *harq_pinfo =
					dect_phy_perf_harq_tx_process_get_by_process_nbr(
						rcv_harq_process_nbr);

				if (!(header->format == DECT_PHY_HEADER_FORMAT_001 &&
				      harq_pinfo->process_in_use)) {
					goto rx_pcc_debug;
				}
				if (header->feedback.format1.format == 1) {
					if (header->feedback.format1.transmission_feedback0) {
						/* ACK: clear the HARQ process resources */
						perf_data.rx_metrics.harq_ack_rx_count++;
					} else {
						/* Handle NACK (with perf cmd we are
						 * not retransmitting)
						 */

						perf_data.rx_metrics.harq_nack_rx_count++;
						dect_phy_perf_tx_total_data_decrease();
					}
					dect_phy_perf_harq_tx_process_release(rcv_harq_process_nbr);
				} else if (header->feedback.format6.format == 6) {
					perf_data.rx_metrics.harq_reset_nack_rx_count++;
					dect_phy_perf_tx_total_data_decrease();
				}
			}
rx_pcc_debug:
			if (cmd_params->debugs) {
				/* Print PCC information in every 5 seconds */
				int64_t z_ticks_last_pcc_print =
					perf_data.rx_metrics.rx_last_pcc_print_zticks;
				int64_t since_last_print_ms =
					k_uptime_delta(&z_ticks_last_pcc_print);

				if (since_last_print_ms >= 5000) {
					char tmp_str[128] = {0};

					dect_common_utils_modem_phy_header_status_to_string_short(
						params->pcc_status.header_status, tmp_str);

					perf_data.rx_metrics.rx_last_pcc_print_zticks =
						z_ticks_last_pcc_print;
					desh_print("Periodic debug: PCC received (time %llu, "
						   "handle %d): status: \"%s\", snr %d, "
						   "RSSI %d, tx pwr %d dbm",
						   params->time, params->pcc_status.handle,
						   tmp_str, params->pcc_status.snr,
						   rssi_level,
						   dect_common_utils_phy_tx_power_to_dbm(
							   phy_h->transmit_power));
				}
			}

			break;
		}
		case DECT_PHY_PERF_EVENT_RX_PDC_DATA: {
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
				uint8_t *pdu_type_ptr = (uint8_t *)params->data;
				uint8_t pdu_type = *pdu_type_ptr & DECT_COMMON_UTILS_BIT_MASK_7BIT;

				if (pdu_type == DECT_MAC_MESSAGE_TYPE_PERF_TX_DATA ||
				    pdu_type == DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_REQ ||
				    pdu_type == DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_RESP) {
					if (perf_data.perf_ongoing) {
						dect_phy_perf_rx_pdc_data_handle(&rcv_params);
					} else {
						desh_print("Perf command TX data received: ignored "
							   "- no perf command running.");
					}
				} else if (pdu_type == DECT_MAC_MESSAGE_TYPE_PERF_HARQ_FEEDBACK) {
					/* desh_print("Perf: HARQ feedback response received."); */
				} else {
					char snum[64] = {0};
					unsigned char hex_data[128];
					int i;
					struct nrf_modem_dect_phy_pdc_event *p_rx_status =
						&(params->rx_status);
					int16_t rssi_level = p_rx_status->rssi_2 / 2;

					desh_print("PDC received (time %llu, handle %d): "
						   "snr %d, RSSI-2 %d "
						   "(RSSI %d), len %d",
						   params->time, p_rx_status->handle,
						   p_rx_status->snr,
						   p_rx_status->rssi_2, rssi_level,
						   params->data_length);

					for (i = 0; i < 64 && i < params->data_length; i++) {
						sprintf(&hex_data[i], "%02x ", params->data[i]);
					}
					hex_data[i + 1] = '\0';
					desh_warn("Perf: received unknown data (type: %s), len %d, "
						  "hex data: %s\n",
						  dect_common_pdu_message_type_to_string(pdu_type,
											 snum),
						  params->data_length, hex_data);
				}
			}
			break;
		}

		default:
			desh_warn("DECT PERF: Unknown event %d received", event.id);
			break;
		}
		k_free(event.data);
	}
}

K_THREAD_DEFINE(dect_phy_perf_th, DECT_PHY_PERF_THREAD_STACK_SIZE, dect_phy_perf_thread_fn, NULL,
		NULL, NULL, K_PRIO_PREEMPT(DECT_PHY_PERF_THREAD_PRIORITY), 0, 0);

/**************************************************************************************************/

static int dect_phy_perf_msgq_data_op_add(uint16_t event_id, void *data, size_t data_size)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);

	event.id = event_id;
	ret = k_msgq_put(&dect_phy_perf_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

static int dect_phy_perf_msgq_non_data_op_add(uint16_t event_id)
{
	int ret = 0;
	struct dect_phy_common_op_event_msgq_item event;

	event.id = event_id;
	event.data = NULL;
	ret = k_msgq_put(&dect_phy_perf_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

void dect_phy_perf_mdm_evt_handler(const struct nrf_modem_dect_phy_event *evt)
{
	dect_app_modem_time_save(&evt->time);

	switch (evt->id) {
	case NRF_MODEM_DECT_PHY_EVT_COMPLETED:
		dect_phy_perf_mdm_op_complete_cb(&evt->op_complete, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC:
		dect_phy_perf_mdm_pcc_cb(&evt->pcc, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PCC_ERROR:
		dect_phy_perf_mdm_pcc_crc_failure_cb(&evt->pcc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC:
		dect_phy_perf_mdm_pdc_cb(&evt->pdc, (uint64_t *) &evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_PDC_ERROR:
		dect_phy_perf_mdm_pdc_crc_failure_cb(&evt->pdc_crc_err, (uint64_t *)&evt->time);
		break;
	case NRF_MODEM_DECT_PHY_EVT_CAPABILITY:
		dect_phy_perf_mdm_capability_get_cb(&evt->capability_get);
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

static void dect_phy_perf_phy_init(void)
{
	int ret = nrf_modem_dect_phy_event_handler_set(dect_phy_perf_mdm_evt_handler);

	if (ret) {
		printk("nrf_modem_dect_phy_event_handler_set returned: %i\n", ret);
	}
}

int dect_phy_perf_cmd_handle(struct dect_phy_perf_params *params)
{
	int ret;

	if (perf_data.perf_ongoing) {
		desh_error("perf command already running");
		return -1;
	}
	dect_phy_perf_phy_init();

	ret = dect_phy_perf_start(params, START);
	if (ret) {
		perf_data.perf_ongoing = false;
	}
	return ret;
}

void dect_phy_perf_cmd_stop(void)
{
	if (!perf_data.perf_ongoing) {
		desh_error("Perf command not running.");
		return;
	}

	if (perf_data.cmd_params.role == DECT_PHY_COMMON_ROLE_CLIENT) {
		int err = 0;

		err = dect_phy_perf_client_report_local_results_and_req_srv_results(NULL);
		if (err) {
			dect_phy_perf_cmd_done();
		} else {
			/* We might receive results from the server */
			perf_data.cmd_params.duration_secs = 0;
		}
	} else {
		dect_phy_perf_msgq_non_data_op_add(DECT_PHY_PERF_EVENT_SERVER_REPORT);
		dect_phy_perf_server_data_ongoing_rx_count_decrease();
		dect_phy_perf_msgq_non_data_op_add(DECT_PHY_PERF_EVENT_CMD_DONE);
	}
}

/**************************************************************************************************/

void dect_phy_perf_rx_on_pcc_crc_failure(void)
{
	if (!perf_data.perf_ongoing) {
		desh_error("receiving perf data but perf not running.");
		return;
	}
	perf_data.rx_metrics.rx_pcc_crc_error_count++;
}

void dect_phy_perf_rx_on_pdc_crc_failure(void)
{
	if (!perf_data.perf_ongoing) {
		desh_error("receiving perf data but perf not running.");
		return;
	}
	perf_data.rx_metrics.rx_pdc_crc_error_count++;
}

static int dect_phy_perf_rx_pdc_data_handle(struct dect_phy_data_rcv_common_params *params)
{
	dect_phy_perf_pdu_t pdu;
	int ret;

	if (!perf_data.perf_ongoing) {
		desh_error("receiving perf data but perf not running.");
		return -1;
	}

	ret = dect_phy_perf_pdu_decode(&pdu, (const uint8_t *)params->data);
	if (ret) {
		perf_data.rx_metrics.rx_decode_error++;
		desh_error("dect_phy_perf_pdu_decode failed: %d", ret);
		return -EBADMSG;
	}
	if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PERF_TX_DATA) {
		perf_data.rx_metrics.rx_testing_mcs = perf_data.rx_metrics.rx_latest_mcs;
		if (perf_data.rx_metrics.rx_total_data_amount == 0) {
			perf_data.rx_metrics.rx_1st_data_received = k_uptime_get();
			perf_data.rx_metrics.rx_last_seq_nbr = pdu.message.tx_data.seq_nbr;
		} else {
			if (pdu.message.tx_data.seq_nbr !=
			    (perf_data.rx_metrics.rx_last_seq_nbr + 1)) {
				struct dect_phy_perf_params *cmd_params = &(perf_data.cmd_params);

				perf_data.rx_metrics.rx_out_of_seq_count++;
				if (cmd_params->debugs) {
					desh_warn("Out of seq in RX: rx_out_of_seq_count %d, "
						  "pdu.seq_nbr %d, perf_data.rx_last_seq_nbr "
						  "%d",
						  perf_data.rx_metrics.rx_out_of_seq_count,
						  pdu.message.tx_data.seq_nbr,
						  perf_data.rx_metrics.rx_last_seq_nbr);
				}
			}
		}
		perf_data.rx_metrics.rx_total_data_amount += params->data_length;
		perf_data.rx_metrics.rx_total_pkt_count++;
		perf_data.rx_metrics.rx_last_seq_nbr = pdu.message.tx_data.seq_nbr;
		perf_data.rx_metrics.rx_last_tx_id = pdu.header.transmitter_id;
		perf_data.rx_metrics.rx_last_data_received = k_uptime_get();
	} else if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_REQ) {
		if (pdu.header.transmitter_id == perf_data.rx_metrics.rx_last_tx_id) {
			desh_print("RESULT_REQ received from tx id %d", pdu.header.transmitter_id);
			dect_phy_perf_msgq_non_data_op_add(DECT_PHY_PERF_EVENT_SERVER_REPORT);
		} else if (pdu.header.transmitter_id ==
				perf_data.rx_metrics.rx_last_tx_id_from_pcc) {
			desh_warn("PERF_RESULTS_REQ received from tx id %d - but no perf "
				  "data received from there.\n"
				  "However, we have seen a PCC lastly from this tx id %d - "
				  "so sending results to there.\n"
				  "Please, check MCS and/or TX PWR on a client side.",
				  pdu.header.transmitter_id,
				  perf_data.rx_metrics.rx_last_tx_id_from_pcc);
			perf_data.rx_metrics.rx_last_tx_id =
				perf_data.rx_metrics.rx_last_tx_id_from_pcc;
			dect_phy_perf_msgq_non_data_op_add(DECT_PHY_PERF_EVENT_SERVER_REPORT);
		} else {
			desh_warn("PERF_RESULTS_REQ received from tx id %d - but no perf "
				  "session with it (last seen tx id %d). "
				  "Check MCS and/or TX PWR on a client side.",
				  pdu.header.transmitter_id, perf_data.rx_metrics.rx_last_tx_id);
		}
	} else if (pdu.header.message_type == DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_RESP) {
		desh_print("Server results received:");
		desh_print("%s", pdu.message.results.results_str);
	} else {
		desh_warn("type %d", pdu.header.message_type);
	}
	return 0;
}

/**************************************************************************************************/
