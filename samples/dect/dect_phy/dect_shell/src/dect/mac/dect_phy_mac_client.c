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

#include <nrf_modem_dect_phy.h>
#include <nrf_modem_at.h>

#include "desh_print.h"
#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_common_pdu.h"
#include "dect_common_settings.h"

#include "dect_phy_api_scheduler.h"

#include "dect_phy_shell.h"
#include "dect_phy_ctrl.h"
#include "dect_phy_mac_pdu.h"
#include "dect_phy_mac_nbr.h"
#include "dect_phy_mac_nbr_bg_scan.h"
#include "dect_phy_mac_cluster_beacon.h"
#include "dect_phy_mac.h"
#include "dect_phy_mac_client.h"

/**************************************************************************************************/

extern struct k_work_q dect_phy_ctrl_work_q;

enum dect_phy_mac_client_association_states {
	DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_DISASSOCIATED,
	DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_WAITING_ASSOCIATION_RESP,
	DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_ASSOCIATED,
};
struct dect_phy_mac_client_association_data {
	enum dect_phy_mac_client_association_states state;
	uint32_t target_long_rd_id;

	uint32_t bg_scan_phy_handle;
	bool bg_scan_ongoing;

	struct dect_phy_mac_nbr_info_list_item *target_nbr;
	struct k_work_delayable association_resp_wait_work;
};
static struct dect_phy_mac_client_data {
	uint16_t client_seq_nbr;
	struct dect_phy_mac_client_association_data associations[DECT_PHY_MAC_MAX_NEIGBORS];

	uint64_t last_tx_time_mdm_ticks;
} client_data = {
	.client_seq_nbr = 0,
	.last_tx_time_mdm_ticks = 0,
};

/**************************************************************************************************/

static int dect_phy_mac_client_data_pdu_encode(struct dect_phy_mac_rach_tx_params *params,
					       uint32_t nw_id_24msb, uint8_t nw_id_8lsb,
					       uint16_t target_short_rd_id,
					       uint8_t **target_ptr, /* In/Out */
					       union nrf_modem_dect_phy_hdr *out_phy_header)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 1, /* calculated later based on needs */
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001, /* No HARQ feedback requested */
		.short_network_id = nw_id_8lsb,
		.transmitter_identity_hi = (uint8_t)(current_settings->common.short_rd_id >> 8),
		.transmitter_identity_lo = (uint8_t)(current_settings->common.short_rd_id & 0xFF),
		.df_mcs = params->mcs,
		.transmit_power = dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm),
		.receiver_identity_hi = (uint8_t)(target_short_rd_id >> 8),
		.receiver_identity_lo = (uint8_t)(target_short_rd_id & 0xFF),
		.feedback.format1.format = 0,
	};
	dect_phy_mac_type_header_t type_header = {
		.version = 0,
		.security = 0, /* 0b00: Not used */
		.type = DECT_PHY_MAC_HEADER_TYPE_PDU,
	};
	dect_phy_mac_common_header_t common_header = {
		.type = DECT_PHY_MAC_HEADER_TYPE_PDU,
		.reset = 1,
		.seq_nbr = client_data.client_seq_nbr++,
		.nw_id = nw_id_24msb, /* 24bit */
		.transmitter_id = current_settings->common.transmitter_id,
	};
	uint8_t *pdu_ptr = *target_ptr;

	pdu_ptr = dect_phy_mac_pdu_type_header_encode(&type_header, pdu_ptr);
	pdu_ptr = dect_phy_mac_pdu_common_header_encode(&common_header, pdu_ptr);

	sys_dlist_t sdu_list;
	dect_phy_mac_sdu_t *data_sdu_list_item =
		(dect_phy_mac_sdu_t *)k_calloc(1, sizeof(dect_phy_mac_sdu_t));
	if (data_sdu_list_item == NULL) {
		return -ENOMEM;
	}
	uint8_t dlc_header_size = DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING_LEN;
	uint16_t dlc_payload_len = strlen(params->tx_data_str) + 1;
	uint16_t payload_data_len = dlc_header_size + dlc_payload_len;

	dect_phy_mac_mux_header_t mux_header1 = {
		.mac_ext = DECT_PHY_MAC_EXT_16BIT_LEN,
		.ie_type = DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW1,
		.payload_length = payload_data_len,
	};

	data_sdu_list_item->mux_header = mux_header1;
	data_sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_DATA_SDU;
	memcpy(data_sdu_list_item->message.data_sdu.data, params->tx_data_str, dlc_payload_len);
	data_sdu_list_item->message.data_sdu.data_length = dlc_payload_len;
	data_sdu_list_item->message.data_sdu.dlc_ie_type =
		DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING;

	sys_dlist_init(&sdu_list);
	sys_dlist_append(&sdu_list, &data_sdu_list_item->dnode);
	pdu_ptr = dect_phy_mac_pdu_sdus_encode(pdu_ptr, &sdu_list);

	/* Length so far  */
	uint16_t encoded_pdu_length = pdu_ptr - *target_ptr;

	header.packet_length = dect_common_utils_phy_packet_length_calculate(
		encoded_pdu_length, header.packet_length_type, header.df_mcs);
	if (header.packet_length < 0) {
		desh_error("(%s): Phy pkt len calculation failed", (__func__));
		return -EINVAL;
	}
	int16_t total_byte_count =
		dect_common_utils_slots_in_bytes(header.packet_length, header.df_mcs);

	if (total_byte_count <= 0) {
		desh_error("Unsupported slot/mcs combination");
		return -EINVAL;
	}
	/* Fill padding if needed */
	int16_t padding_need = total_byte_count - encoded_pdu_length;

	int err = dect_phy_mac_pdu_sdu_list_add_padding(&pdu_ptr, &sdu_list, padding_need);

	if (err) {
		desh_warn("(%s): Failed to add padding: err %d (continue)", __func__, err);
	}

	*target_ptr = pdu_ptr;

	union nrf_modem_dect_phy_hdr phy_header;

	memcpy(out_phy_header, &header, sizeof(phy_header.type_2));

	return header.packet_length;
}

static uint64_t dect_phy_mac_client_next_rach_tx_time_get(
	struct dect_phy_mac_nbr_info_list_item *target_nbr)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	uint64_t time_now = dect_app_modem_time_now();
	uint64_t beacon_received = target_nbr->time_rcvd_mdm_ticks;
	uint64_t first_possible_tx = time_now + dect_phy_ctrl_modem_latency_for_next_op_get(true) +
		(US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));
	uint64_t next_beacon_frame_start, beacon_interval_mdm_ticks, ra_start_mdm_ticks;
	uint64_t ra_interval_mdm_ticks, last_valid_rach_rx_frame_time;

	int32_t beacon_interval_ms = dect_phy_mac_pdu_cluster_beacon_period_in_ms(
		target_nbr->beacon_msg.cluster_beacon_period);

	if (beacon_interval_ms < 0) {
		return 0;
	}
	bool our_beacon_is_running = dect_phy_mac_cluster_beacon_is_running();

	if (our_beacon_is_running) {
		/* If our beacon is running, send beyond the window to get the priority over others.
		 * This adds more delay to TX but is more reliable to avoid scheduling collisions.
		 */
		first_possible_tx += MS_TO_MODEM_TICKS(DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS);
	}

	beacon_interval_mdm_ticks = MS_TO_MODEM_TICKS(beacon_interval_ms);

	/* We are sending after next beacon and ...  */
	next_beacon_frame_start = beacon_received;
	while (next_beacon_frame_start < first_possible_tx) {
		next_beacon_frame_start += beacon_interval_mdm_ticks;
	}

	if (target_nbr->ra_ie.repeat == DECT_PHY_MAC_RA_REPEAT_TYPE_SINGLE) {
		ra_interval_mdm_ticks = beacon_interval_mdm_ticks;
		last_valid_rach_rx_frame_time = next_beacon_frame_start;
	} else if (target_nbr->ra_ie.repeat == DECT_PHY_MAC_RA_REPEAT_TYPE_FRAMES) {
		ra_interval_mdm_ticks =
			target_nbr->ra_ie.repetition * DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS;
		last_valid_rach_rx_frame_time =
			next_beacon_frame_start +
			(target_nbr->ra_ie.validity * DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS);
	} else {
		__ASSERT_NO_MSG(target_nbr->ra_ie.repeat == DECT_PHY_MAC_RA_REPEAT_TYPE_SUBSLOTS);
		ra_interval_mdm_ticks =
			target_nbr->ra_ie.repetition * DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;
		last_valid_rach_rx_frame_time =
			next_beacon_frame_start +
			(target_nbr->ra_ie.validity * DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS);
	}

	/* .... and after the last client TX */
	if (client_data.last_tx_time_mdm_ticks > first_possible_tx) {
		first_possible_tx = client_data.last_tx_time_mdm_ticks + 1;
	}

	/* ... and try to avoid collisions with our own beacon TX:  */
	uint64_t next_our_beacon_frame_time = 0;

	if (dect_phy_mac_cluster_beacon_is_running()) {
		next_our_beacon_frame_time =
			dect_phy_mac_cluster_beacon_last_tx_frame_time_get();
		uint32_t our_beacon_interval_mdm_ticks =
			MS_TO_MODEM_TICKS(DECT_PHY_MAC_CLUSTER_BEACON_INTERVAL_MS);

		while (next_our_beacon_frame_time < first_possible_tx) {
			next_our_beacon_frame_time += our_beacon_interval_mdm_ticks;
		}
	}

	/* ...and in 1st RA (if not associated). Additionally, to get RX really on target:
	 * delay TX by 2 subslots.
	 */
	ra_start_mdm_ticks = next_beacon_frame_start + ((target_nbr->ra_ie.start_subslot + 2) *
							DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);

	if (dect_phy_mac_client_associated_by_target_short_rd_id(target_nbr->short_rd_id)) {
		/* To avoid collision with a background scan that is done with associated client */
		ra_start_mdm_ticks += ra_interval_mdm_ticks;
	}

	while (ra_start_mdm_ticks < first_possible_tx &&
	       ra_start_mdm_ticks < last_valid_rach_rx_frame_time) {
		ra_start_mdm_ticks += ra_interval_mdm_ticks;
		if (next_our_beacon_frame_time && dect_common_utils_mdm_ticks_is_in_range(
							ra_start_mdm_ticks,
							next_our_beacon_frame_time,
							next_our_beacon_frame_time +
							(DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS /
								2))) {
			ra_start_mdm_ticks += ra_interval_mdm_ticks;
		}
	}

	return ra_start_mdm_ticks;
}

static int dect_phy_mac_client_rach_tx(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				struct dect_phy_mac_rach_tx_params *params);

struct dect_phy_mac_client_rach_tx_data {
	struct k_work_delayable work;
	struct dect_phy_mac_rach_tx_params cmd_params;
	struct dect_phy_mac_nbr_info_list_item target_nbr;
};
static struct dect_phy_mac_client_rach_tx_data client_rach_tx_work_data;

static void dect_phy_mac_client_rach_tx_worker(struct k_work *work_item)
{
	struct k_work_delayable *delayable_work = k_work_delayable_from_work(work_item);
	struct dect_phy_mac_client_rach_tx_data *data =
		CONTAINER_OF(delayable_work, struct dect_phy_mac_client_rach_tx_data, work);
	struct dect_phy_mac_rach_tx_params cmd_params;
	int err;

	cmd_params = data->cmd_params;

	if (data->cmd_params.get_mdm_temp) {
		char tmp_str[DECT_DATA_MAX_LEN * 2] = {0};
		int mdm_temperature = dect_phy_ctrl_modem_temperature_get();

		/* Modem temperature to be included in a data JSON */
		if (mdm_temperature == NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED) {
			sprintf(tmp_str, "{\"data\":\"%s\",\"m_tmp\":\"N/A\"}",
				cmd_params.tx_data_str);
		} else {
			sprintf(tmp_str, "{\"data\":\"%s\",\"m_tmp\":\"%d\"}",
				cmd_params.tx_data_str, mdm_temperature);
		}
		memset(cmd_params.tx_data_str, 0, DECT_DATA_MAX_LEN);
		strncpy(cmd_params.tx_data_str, tmp_str, DECT_DATA_MAX_LEN - 1);
	}

	/* Get fresh nbr info */
	struct dect_phy_mac_nbr_info_list_item *scan_info =
		dect_phy_mac_nbr_info_get_by_long_rd_id(cmd_params.target_long_rd_id);

	if (!scan_info) {
		desh_warn("(%s): Beacon with long RD ID %u has not been seen in scan results",
			(__func__), cmd_params.target_long_rd_id);
		return;
	}

	data->target_nbr = *scan_info;
	err = dect_phy_mac_client_rach_tx(&data->target_nbr, &cmd_params);
	if (err) {
		desh_error("(%s): client_rach_tx failed: %d", (__func__), err);
	}

	if (cmd_params.interval_secs) {
		k_work_schedule_for_queue(&dect_phy_ctrl_work_q,
					  &client_rach_tx_work_data.work,
					  K_SECONDS(cmd_params.interval_secs));
	}
}

int dect_phy_mac_client_rach_tx_start(
	struct dect_phy_mac_nbr_info_list_item *target_nbr,
	struct dect_phy_mac_rach_tx_params *params)
{
	client_rach_tx_work_data.cmd_params = *params;
	client_rach_tx_work_data.target_nbr = *target_nbr;
	k_work_schedule_for_queue(
		&dect_phy_ctrl_work_q, &client_rach_tx_work_data.work, K_SECONDS(0));

	return 0;
}

void dect_mac_client_rach_tx_stop(void)
{
	k_work_cancel_delayable(&client_rach_tx_work_data.work);
}

static int dect_phy_mac_client_rach_tx(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				struct dect_phy_mac_rach_tx_params *params)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	uint64_t beacon_received = target_nbr->time_rcvd_mdm_ticks;
	uint64_t ra_start_mdm_ticks;

	uint32_t beacon_interval_ms = dect_phy_mac_pdu_cluster_beacon_period_in_ms(
		target_nbr->beacon_msg.cluster_beacon_period);

	union nrf_modem_dect_phy_hdr phy_header;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN];
	uint8_t *pdu_ptr = encoded_data_to_send;
	int ret;
	uint8_t slot_count = 0;

	memset(encoded_data_to_send, 0, DECT_DATA_MAX_LEN);

	/* Encode data PDU to be sent */
	ret = dect_phy_mac_client_data_pdu_encode(params, target_nbr->nw_id_24msb,
						  target_nbr->nw_id_8lsb, target_nbr->short_rd_id,
						  &pdu_ptr, &phy_header);
	if (ret < 0) {
		desh_error("(%s): Failed to encode client data pdu", __func__);
		return ret;
	}
	slot_count = ret + 1;

	ra_start_mdm_ticks = dect_phy_mac_client_next_rach_tx_time_get(target_nbr);
	if (ra_start_mdm_ticks == 0) {
		desh_error("(%s): Failed to get next RACH TX time", __func__);
		return -EINVAL;
	}

	struct dect_phy_api_scheduler_list_item_config *sched_list_item_conf;
	struct dect_phy_api_scheduler_list_item *sched_list_item =
		dect_phy_api_scheduler_list_item_alloc_tx_element(&sched_list_item_conf);

	if (!sched_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_alloc_tx_element failed: No "
			   "memory to TX a data to FT");
		return -ENOMEM;
	}
	uint16_t encoded_pdu_length = pdu_ptr - encoded_data_to_send;

	sched_list_item_conf->address_info.network_id = target_nbr->nw_id_32bit;
	sched_list_item_conf->address_info.transmitter_long_rd_id =
		current_settings->common.transmitter_id;
	sched_list_item_conf->address_info.receiver_long_rd_id = params->target_long_rd_id;

	sched_list_item_conf->cb_op_completed = NULL;

	sched_list_item_conf->channel = target_nbr->channel;
	sched_list_item_conf->frame_time = ra_start_mdm_ticks;
	sched_list_item_conf->start_slot = 0;

	client_data.last_tx_time_mdm_ticks = ra_start_mdm_ticks;

	sched_list_item_conf->interval_mdm_ticks = 0;
	sched_list_item_conf->length_slots = slot_count;
	sched_list_item_conf->length_subslots = 0;

	sched_list_item_conf->tx.phy_lbt_period = NRF_MODEM_DECT_LBT_PERIOD_MIN;
	sched_list_item_conf->tx.phy_lbt_rssi_threshold_max =
		current_settings->rssi_scan.busy_threshold;

	sched_list_item_conf->tx.harq_feedback_requested = false;

	sched_list_item->sched_config.tx.encoded_payload_pdu_size = encoded_pdu_length;
	memcpy(sched_list_item->sched_config.tx.encoded_payload_pdu, encoded_data_to_send,
	       sched_list_item->sched_config.tx.encoded_payload_pdu_size);

	sched_list_item->sched_config.tx.header_type = DECT_PHY_HEADER_TYPE2;
	memcpy(&sched_list_item->sched_config.tx.phy_header.type_2, &phy_header.type_2,
	       sizeof(phy_header.type_2));

	sched_list_item->priority = DECT_PRIORITY0_FORCE_TX;
	sched_list_item->phy_op_handle = DECT_PHY_MAC_CLIENT_RA_TX_HANDLE;

	if (params->interval_secs) {
		sched_list_item->phy_op_handle = DECT_PHY_MAC_CLIENT_RA_TX_CONTINUOUS_HANDLE;
	}

	/* Add tx operation to scheduler list */
	if (!dect_phy_api_scheduler_list_item_add(sched_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed", (__func__));
		dect_phy_api_scheduler_list_item_dealloc(sched_list_item);
		return -EBUSY;
	}
	desh_print("Scheduled random access data TX:\n"
		   "  target long rd id %u (0x%08x), short rd id %u (0x%04x),\n"
		   "  target 32bit nw id %u (0x%08x), tx pwr %d dbm,\n"
		   "  channel %d, payload PDU byte count: %d,\n"
		   "  beacon interval %d, frame time %lld, beacon received %lld",
		   params->target_long_rd_id, params->target_long_rd_id, target_nbr->short_rd_id,
		   target_nbr->short_rd_id, target_nbr->nw_id_32bit, target_nbr->nw_id_32bit,
		   params->tx_power_dbm, target_nbr->channel, encoded_pdu_length,
		   beacon_interval_ms, sched_list_item_conf->frame_time, beacon_received);

	return 0;
}

/**************************************************************************************************/

static int dect_phy_mac_client_association_req_pdu_encode(
	struct dect_phy_mac_associate_params *params, uint32_t nw_id_24msb,
	uint8_t nw_id_8lsb, uint16_t target_short_rd_id, uint8_t **target_ptr, /* In/Out */
	union nrf_modem_dect_phy_hdr *out_phy_header)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 1, /* calculated later based on needs */
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001, /* No HARQ feedback requested */
		.short_network_id = nw_id_8lsb,
		.transmitter_identity_hi = (uint8_t)(current_settings->common.short_rd_id >> 8),
		.transmitter_identity_lo = (uint8_t)(current_settings->common.short_rd_id & 0xFF),
		.df_mcs = params->mcs,
		.transmit_power = dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm),
		.receiver_identity_hi = (uint8_t)(target_short_rd_id >> 8),
		.receiver_identity_lo = (uint8_t)(target_short_rd_id & 0xFF),
		.feedback.format1.format = 0,
	};
	dect_phy_mac_type_header_t type_header = {
		.version = 0,
		.security = 0, /* 0b00: Not used */
		.type = DECT_PHY_MAC_HEADER_TYPE_UNICAST,
	};
	dect_phy_mac_common_header_t common_header = {
		.type = DECT_PHY_MAC_HEADER_TYPE_UNICAST,
		.reset = 1,
		.seq_nbr = client_data.client_seq_nbr++,
		.nw_id = nw_id_24msb, /* 24bit */
		.transmitter_id = current_settings->common.transmitter_id,
		.receiver_id = params->target_long_rd_id,
	};
	uint8_t *pdu_ptr = *target_ptr;

	pdu_ptr = dect_phy_mac_pdu_type_header_encode(&type_header, pdu_ptr);
	pdu_ptr = dect_phy_mac_pdu_common_header_encode(&common_header, pdu_ptr);

	sys_dlist_t sdu_list;
	dect_phy_mac_sdu_t *data_sdu_list_item =
		(dect_phy_mac_sdu_t *)k_calloc(1, sizeof(dect_phy_mac_sdu_t));
	if (data_sdu_list_item == NULL) {
		return -ENOMEM;
	}
	uint16_t payload_data_len = DECT_PHY_MAC_ASSOCIATION_REQ_MIN_LEN;
	dect_phy_mac_mux_header_t mux_header1 = {
		.mac_ext = DECT_PHY_MAC_EXT_8BIT_LEN,
		.ie_type = DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REQ,
		.payload_length = payload_data_len,
	};
	dect_phy_mac_association_req_t association_req = {
		.pwr_const_bit = 0,
		.ft_mode_bit = 0,
		.current_cluster_channel_bit = 0,
		.setup_cause = DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_INIT,
		.flow_count = 1,
		.harq_tx_process_count = 1,
		.max_harq_tx_retransmission_delay = 4, /* Coded value: 1ms */
		.harq_rx_process_count = 1,
		.max_harq_rx_retransmission_delay = 4, /* Coded value: 1ms */
		/* The same that client_rach_tx uses: */
		.flow_id = DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW1,
	};

	data_sdu_list_item->mux_header = mux_header1;
	data_sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REQ;
	data_sdu_list_item->message.association_req = association_req;

	sys_dlist_init(&sdu_list);
	sys_dlist_append(&sdu_list, &data_sdu_list_item->dnode);
	pdu_ptr = dect_phy_mac_pdu_sdus_encode(pdu_ptr, &sdu_list);

	/* Length so far  */
	uint16_t encoded_pdu_length = pdu_ptr - *target_ptr;

	header.packet_length = dect_common_utils_phy_packet_length_calculate(
		encoded_pdu_length, header.packet_length_type, header.df_mcs);
	if (header.packet_length < 0) {
		desh_error("(%s): Phy pkt len calculation failed", (__func__));
		return -EINVAL;
	}
	int16_t total_byte_count =
		dect_common_utils_slots_in_bytes(header.packet_length, header.df_mcs);

	if (total_byte_count <= 0) {
		desh_error("Unsupported slot/mcs combination");
		return -EINVAL;
	}
	/* Fill padding if needed */
	int16_t padding_need = total_byte_count - encoded_pdu_length;

	int err = dect_phy_mac_pdu_sdu_list_add_padding(&pdu_ptr, &sdu_list, padding_need);

	if (err) {
		desh_warn("(%s): Failed to add padding: err %d (continue)", __func__, err);
	}

	*target_ptr = pdu_ptr;

	union nrf_modem_dect_phy_hdr phy_header;

	memcpy(out_phy_header, &header, sizeof(phy_header.type_2));

	return header.packet_length;
}

static int dect_phy_mac_client_associate_msg_send(
	struct dect_phy_mac_nbr_info_list_item *target_nbr,
	struct dect_phy_mac_associate_params *params)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	uint64_t beacon_received = target_nbr->time_rcvd_mdm_ticks;
	uint64_t ra_start_mdm_ticks;

	uint32_t beacon_interval_ms = dect_phy_mac_pdu_cluster_beacon_period_in_ms(
		target_nbr->beacon_msg.cluster_beacon_period);

	union nrf_modem_dect_phy_hdr phy_header;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN];
	uint8_t *pdu_ptr = encoded_data_to_send;
	int ret;
	uint8_t slot_count = 0;

	memset(encoded_data_to_send, 0, DECT_DATA_MAX_LEN);

	/* Encode data PDU to be sent */
	ret = dect_phy_mac_client_association_req_pdu_encode(
		params, target_nbr->nw_id_24msb, target_nbr->nw_id_8lsb, target_nbr->short_rd_id,
		&pdu_ptr, &phy_header);
	if (ret < 0) {
		desh_error("(%s): Failed to encode a association req", __func__);
		return ret;
	}
	slot_count = ret + 1;

	ra_start_mdm_ticks = dect_phy_mac_client_next_rach_tx_time_get(target_nbr);
	if (ra_start_mdm_ticks == 0) {
		desh_error("(%s): Failed to get next RACH TX time", __func__);
		return -EINVAL;
	}

	struct dect_phy_api_scheduler_list_item_config *sched_list_item_conf;
	struct dect_phy_api_scheduler_list_item *sched_list_item =
		dect_phy_api_scheduler_list_item_alloc_tx_element(&sched_list_item_conf);

	if (!sched_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_alloc_tx_element failed: No "
			   "memory to TX a association req");
		return -ENOMEM;
	}
	uint16_t encoded_pdu_length = pdu_ptr - encoded_data_to_send;

	sched_list_item_conf->address_info.network_id = target_nbr->nw_id_32bit;
	sched_list_item_conf->address_info.transmitter_long_rd_id =
		current_settings->common.transmitter_id;
	sched_list_item_conf->address_info.receiver_long_rd_id = params->target_long_rd_id;

	sched_list_item_conf->cb_op_completed = NULL;

	sched_list_item_conf->channel = target_nbr->channel;
	sched_list_item_conf->frame_time = ra_start_mdm_ticks;
	sched_list_item_conf->start_slot = 0;

	client_data.last_tx_time_mdm_ticks = ra_start_mdm_ticks;

	sched_list_item_conf->interval_mdm_ticks = 0;
	sched_list_item_conf->length_slots = slot_count;
	sched_list_item_conf->length_subslots = 0;

	sched_list_item_conf->tx.phy_lbt_period = NRF_MODEM_DECT_LBT_PERIOD_MIN;
	sched_list_item_conf->tx.phy_lbt_rssi_threshold_max =
		current_settings->rssi_scan.busy_threshold;

	sched_list_item_conf->tx.harq_feedback_requested = false;

	sched_list_item->sched_config.tx.encoded_payload_pdu_size = encoded_pdu_length;
	memcpy(sched_list_item->sched_config.tx.encoded_payload_pdu, encoded_data_to_send,
	       sched_list_item->sched_config.tx.encoded_payload_pdu_size);

	sched_list_item->sched_config.tx.header_type = DECT_PHY_HEADER_TYPE2;
	memcpy(&sched_list_item->sched_config.tx.phy_header.type_2, &phy_header.type_2,
	       sizeof(phy_header.type_2));

	sched_list_item->priority = DECT_PRIORITY0_FORCE_TX;
	sched_list_item->phy_op_handle = DECT_PHY_MAC_CLIENT_ASSOCIATION_TX_HANDLE;

	/* Add tx operation to scheduler list */
	if (!dect_phy_api_scheduler_list_item_add(sched_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed", (__func__));
		dect_phy_api_scheduler_list_item_dealloc(sched_list_item);
		return -EBUSY;
	}

	/* Schedule also RX for catching a response */
	struct dect_phy_api_scheduler_list_item_config *rach_list_item_conf;

	sched_list_item = dect_phy_api_scheduler_list_item_alloc_rx_element(&rach_list_item_conf);
	if (!sched_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_alloc_rx_element failed",
			   (__func__));
		ret = -ENOMEM;
		goto err_exit;
	}
	/* For the response: To be compliant with both type DECT_Delays: we are starting RX asap and
	 * keep it open longer
	 */
	uint64_t rx_time = ra_start_mdm_ticks +
			   (slot_count * DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS) +
			   (3 * DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);

	rach_list_item_conf->cb_op_completed = NULL;
	rach_list_item_conf->channel = target_nbr->channel;
	rach_list_item_conf->frame_time = rx_time;
	rach_list_item_conf->start_slot = 0;

	rach_list_item_conf->rx.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS;
	rach_list_item_conf->rx.expected_rssi_level = 0;
	rach_list_item_conf->rx.duration = 2 * DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS;

	rach_list_item_conf->rx.network_id = target_nbr->nw_id_32bit;

	sched_list_item_conf->address_info.network_id = target_nbr->nw_id_32bit;
	sched_list_item_conf->address_info.transmitter_long_rd_id =
		current_settings->common.transmitter_id;
	sched_list_item_conf->address_info.receiver_long_rd_id = target_nbr->long_rd_id;

	/* Only receive the ones destinated to this device: */
	rach_list_item_conf->rx.filter.is_short_network_id_used = true;
	rach_list_item_conf->rx.filter.short_network_id = target_nbr->nw_id_8lsb;
	rach_list_item_conf->rx.filter.receiver_identity = current_settings->common.short_rd_id;

	sched_list_item->priority = DECT_PRIORITY0_FORCE_RX;
	sched_list_item->phy_op_handle = DECT_PHY_MAC_CLIENT_ASSOCIATION_RX_HANDLE;

	if (!dect_phy_api_scheduler_list_item_add(sched_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add for RACH failed",
			   (__func__));
		ret = -EBUSY;
		dect_phy_api_scheduler_list_item_dealloc(sched_list_item);
	}

	desh_print("Scheduled random access data TX/RX:\n"
		   "  target long rd id %u (0x%08x), short rd id %u (0x%04x),\n"
		   "  target 32bit nw id %u (0x%08x), tx pwr %d dbm,\n"
		   "  channel %d, payload PDU byte count: %d,\n"
		   "  beacon interval %d, frame time %lld, beacon received %lld",
		   params->target_long_rd_id, params->target_long_rd_id, target_nbr->short_rd_id,
		   target_nbr->short_rd_id, target_nbr->nw_id_32bit, target_nbr->nw_id_32bit,
		   params->tx_power_dbm, target_nbr->channel, encoded_pdu_length,
		   beacon_interval_ms, sched_list_item_conf->frame_time, beacon_received);

	return 0;
err_exit:
	return -1;
}

static struct dect_phy_mac_client_association_data *dect_phy_mac_client_free_association_get(void)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (client_data.associations[i].state ==
		    DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_DISASSOCIATED) {
			return &client_data.associations[i];
		}
	}
	return NULL;
}

static void dect_phy_mac_client_association_data_init(void)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		client_data.associations[i].state =
			DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_DISASSOCIATED;
		client_data.associations[i].target_long_rd_id = 0;
		client_data.associations[i].bg_scan_phy_handle =
			DECT_PHY_MAC_CLIENT_ASSOCIATED_BG_SCAN + i;
		client_data.associations[i].target_nbr = NULL;
		client_data.associations[i].bg_scan_ongoing = false;
	}
}

static struct dect_phy_mac_client_association_data *dect_phy_mac_client_association_data_get(
	uint32_t target_long_rd_id)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (client_data.associations[i].target_long_rd_id == target_long_rd_id) {
			return &client_data.associations[i];
		}
	}
	return NULL;
}

static void dect_phy_mac_client_associate_resp_timeout_worker(struct k_work *work_item)
{
	struct k_work_delayable *delayable_work = k_work_delayable_from_work(work_item);
	struct dect_phy_mac_client_association_data *association_data =
		CONTAINER_OF(delayable_work, struct dect_phy_mac_client_association_data,
			     association_resp_wait_work);

	if (association_data->state !=
		DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_WAITING_ASSOCIATION_RESP) {
		return;
	}

	association_data->target_nbr = NULL;
	association_data->state = DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_DISASSOCIATED;
	dect_phy_mac_nbr_bg_scan_stop(association_data->bg_scan_phy_handle);
	association_data->bg_scan_ongoing = false;

	desh_warn("Association response timeout: not associated with device long RD ID %d",
		association_data->target_long_rd_id);
}

int dect_phy_mac_client_associate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				  struct dect_phy_mac_associate_params *params)
{
	struct dect_phy_mac_client_association_data *association_data = NULL;
	int err;

	association_data = dect_phy_mac_client_association_data_get(params->target_long_rd_id);
	if (association_data == NULL) {
		association_data = dect_phy_mac_client_free_association_get();
		if (association_data == NULL) {
			desh_error("(%s): Max amount of associated clients", (__func__));
			return -EINVAL;
		}
	} else {
		desh_warn("(%s): Association exists for target long rd id %u - continue",
			(__func__), params->target_long_rd_id);
		association_data->target_nbr = target_nbr;
	}
	err = dect_phy_mac_client_associate_msg_send(target_nbr, params);
	if (err) {
		desh_error("(%s): dect_phy_mac_client_associate_msg_send failed: %d",
			(__func__), err);
		return err;
	}
	association_data->state = DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_WAITING_ASSOCIATION_RESP;
	association_data->target_long_rd_id = params->target_long_rd_id;
	association_data->target_nbr = target_nbr;

	k_work_init_delayable(
		&association_data->association_resp_wait_work,
		dect_phy_mac_client_associate_resp_timeout_worker);

	/* Start worker for waiting association response */
	k_work_schedule_for_queue(&dect_phy_ctrl_work_q,
				  &association_data->association_resp_wait_work,
				  K_SECONDS(DECT_PHY_MAC_CLIENT_ASSOCIATION_RESP_WAIT_TIME_SEC));
	return 0;
}

void dect_phy_mac_client_nbr_scan_completed_cb(
	struct dect_phy_mac_nbr_bg_scan_op_completed_info *info)
{
	struct dect_phy_mac_client_association_data *association_data =
		dect_phy_mac_client_association_data_get(info->target_long_rd_id);

	if (!association_data) {
		desh_warn("(%s): No association data found for target long rd id %u",
			(__func__), info->target_long_rd_id);
		return;
	}
	if (info->cause == DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_USER_INITIATED) {
		desh_print("(%s): Background scan completed for target long rd id %u. "
			   "Cause: user initiated",
			(__func__), info->target_long_rd_id);
	} else {
		__ASSERT_NO_MSG(info->cause ==
			DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_UNREACHABLE);
		desh_warn("(%s): Background scan completed for target long rd id %u. "
			   "Cause: unreachable for a %d msecs.",
			(__func__), info->target_long_rd_id,
			DECT_PHY_MAC_NBR_BG_SCAN_MAX_UNREACHABLE_TIME_MS);
	}
	association_data->bg_scan_ongoing = false;
}

void dect_phy_mac_client_associate_resp_handle(
	dect_phy_mac_common_header_t *common_header,
	dect_phy_mac_association_resp_t *association_resp)
{
	struct dect_phy_mac_client_association_data *association_data =
		dect_phy_mac_client_association_data_get(common_header->transmitter_id);

	if (!association_data) {
		desh_warn("(%s): No association data found for transmitter id %u",
			(__func__), common_header->transmitter_id);
		return;
	}
	k_work_cancel_delayable(&association_data->association_resp_wait_work);

	association_data->state =  DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_ASSOCIATED;
	desh_print("(%s): associated with device %d - starting background scan",
		(__func__), common_header->transmitter_id);

	struct dect_phy_mac_nbr_bg_scan_params bg_scan_params;

	bg_scan_params.cb_op_completed = dect_phy_mac_client_nbr_scan_completed_cb;
	bg_scan_params.target_nbr = association_data->target_nbr;
	bg_scan_params.target_long_rd_id = association_data->target_long_rd_id;
	bg_scan_params.phy_op_handle = association_data->bg_scan_phy_handle;

	if (association_data->bg_scan_ongoing) {
		dect_phy_mac_nbr_bg_scan_stop(association_data->bg_scan_phy_handle);
	}

	if (dect_phy_mac_nbr_bg_scan_start(&bg_scan_params)) {
		desh_warn("(%s): dect_phy_mac_nbr_bg_scan_start failed", (__func__));
	} else {
		association_data->bg_scan_ongoing = true;
	}
}

/**************************************************************************************************/

static int dect_phy_mac_client_association_rel_pdu_encode(
	struct dect_phy_mac_associate_params *params, uint32_t nw_id_24msb,
	uint8_t nw_id_8lsb, uint16_t target_short_rd_id, uint8_t **target_ptr, /* In/Out */
	union nrf_modem_dect_phy_hdr *out_phy_header)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	struct dect_phy_header_type2_format1_t header = {
		.packet_length = 1, /* calculated later based on needs */
		.packet_length_type = DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS,
		.format = DECT_PHY_HEADER_FORMAT_001, /* No HARQ feedback requested */
		.short_network_id = nw_id_8lsb,
		.transmitter_identity_hi = (uint8_t)(current_settings->common.short_rd_id >> 8),
		.transmitter_identity_lo = (uint8_t)(current_settings->common.short_rd_id & 0xFF),
		.df_mcs = params->mcs,
		.transmit_power = dect_common_utils_dbm_to_phy_tx_power(params->tx_power_dbm),
		.receiver_identity_hi = (uint8_t)(target_short_rd_id >> 8),
		.receiver_identity_lo = (uint8_t)(target_short_rd_id & 0xFF),
		.feedback.format1.format = 0,
	};
	dect_phy_mac_type_header_t type_header = {
		.version = 0,
		.security = 0, /* 0b00: Not used */
		.type = DECT_PHY_MAC_HEADER_TYPE_UNICAST,
	};
	dect_phy_mac_common_header_t common_header = {
		.type = DECT_PHY_MAC_HEADER_TYPE_UNICAST,
		.reset = 1,
		.seq_nbr = client_data.client_seq_nbr++,
		.nw_id = nw_id_24msb, /* 24bit */
		.transmitter_id = current_settings->common.transmitter_id,
		.receiver_id = params->target_long_rd_id,
	};
	uint8_t *pdu_ptr = *target_ptr;

	pdu_ptr = dect_phy_mac_pdu_type_header_encode(&type_header, pdu_ptr);
	pdu_ptr = dect_phy_mac_pdu_common_header_encode(&common_header, pdu_ptr);

	sys_dlist_t sdu_list;
	dect_phy_mac_sdu_t *data_sdu_list_item =
		(dect_phy_mac_sdu_t *)k_calloc(1, sizeof(dect_phy_mac_sdu_t));
	if (data_sdu_list_item == NULL) {
		return -ENOMEM;
	}
	uint16_t payload_data_len = DECT_PHY_MAC_ASSOCIATION_REL_LEN;
	dect_phy_mac_mux_header_t mux_header1 = {
		.mac_ext = DECT_PHY_MAC_EXT_8BIT_LEN,
		.ie_type = DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REL,
		.payload_length = payload_data_len,
	};
	dect_phy_mac_association_rel_t association_rel = {
		.rel_cause = DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_CONN_TERMINATION,
	};

	data_sdu_list_item->mux_header = mux_header1;
	data_sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REL;
	data_sdu_list_item->message.association_rel = association_rel;

	sys_dlist_init(&sdu_list);
	sys_dlist_append(&sdu_list, &data_sdu_list_item->dnode);
	pdu_ptr = dect_phy_mac_pdu_sdus_encode(pdu_ptr, &sdu_list);

	/* Length so far  */
	uint16_t encoded_pdu_length = pdu_ptr - *target_ptr;

	header.packet_length = dect_common_utils_phy_packet_length_calculate(
		encoded_pdu_length, header.packet_length_type, header.df_mcs);
	if (header.packet_length < 0) {
		desh_error("(%s): Phy pkt len calculation failed", (__func__));
		return -EINVAL;
	}
	int16_t total_byte_count =
		dect_common_utils_slots_in_bytes(header.packet_length, header.df_mcs);

	if (total_byte_count <= 0) {
		desh_error("Unsupported slot/mcs combination");
		return -EINVAL;
	}
	/* Fill padding if needed */
	int16_t padding_need = total_byte_count - encoded_pdu_length;
	int err = dect_phy_mac_pdu_sdu_list_add_padding(&pdu_ptr, &sdu_list, padding_need);

	if (err) {
		desh_warn("(%s): Failed to add padding: err %d (continue)", __func__, err);
	}
	*target_ptr = pdu_ptr;

	union nrf_modem_dect_phy_hdr phy_header;

	memcpy(out_phy_header, &header, sizeof(phy_header.type_2));

	return header.packet_length;
}

static int dect_phy_mac_client_dissociate_msg_send(
	struct dect_phy_mac_nbr_info_list_item *target_nbr,
	struct dect_phy_mac_associate_params *params)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	uint64_t beacon_received = target_nbr->time_rcvd_mdm_ticks;
	uint64_t ra_start_mdm_ticks;

	uint32_t beacon_interval_ms = dect_phy_mac_pdu_cluster_beacon_period_in_ms(
		target_nbr->beacon_msg.cluster_beacon_period);

	union nrf_modem_dect_phy_hdr phy_header;
	uint8_t encoded_data_to_send[DECT_DATA_MAX_LEN];
	uint8_t *pdu_ptr = encoded_data_to_send;
	int ret;
	uint8_t slot_count = 0;

	memset(encoded_data_to_send, 0, DECT_DATA_MAX_LEN);

	/* Encode data PDU to be sent */
	ret = dect_phy_mac_client_association_rel_pdu_encode(
		params, target_nbr->nw_id_24msb, target_nbr->nw_id_8lsb, target_nbr->short_rd_id,
		&pdu_ptr, &phy_header);
	if (ret < 0) {
		desh_error("(%s): Failed to encode association release req", __func__);
		return ret;
	}
	slot_count = ret + 1;

	ra_start_mdm_ticks = dect_phy_mac_client_next_rach_tx_time_get(target_nbr);
	if (ra_start_mdm_ticks == 0) {
		desh_error("(%s): Failed to get next RACH TX time", __func__);
		return -EINVAL;
	}

	struct dect_phy_api_scheduler_list_item_config *sched_list_item_conf;
	struct dect_phy_api_scheduler_list_item *sched_list_item =
		dect_phy_api_scheduler_list_item_alloc_tx_element(&sched_list_item_conf);

	if (!sched_list_item) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_alloc_tx_element failed: No "
			   "memory to TX a association release req");
		return -ENOMEM;
	}
	uint16_t encoded_pdu_length = pdu_ptr - encoded_data_to_send;

	sched_list_item_conf->address_info.network_id = target_nbr->nw_id_32bit;
	sched_list_item_conf->address_info.transmitter_long_rd_id =
		current_settings->common.transmitter_id;
	sched_list_item_conf->address_info.receiver_long_rd_id = params->target_long_rd_id;

	sched_list_item_conf->cb_op_completed = NULL;

	sched_list_item_conf->channel = target_nbr->channel;
	sched_list_item_conf->frame_time = ra_start_mdm_ticks;
	sched_list_item_conf->start_slot = 0;

	client_data.last_tx_time_mdm_ticks = ra_start_mdm_ticks;

	sched_list_item_conf->interval_mdm_ticks = 0;
	sched_list_item_conf->length_slots = slot_count;
	sched_list_item_conf->length_subslots = 0;

	sched_list_item_conf->tx.phy_lbt_period = NRF_MODEM_DECT_LBT_PERIOD_MIN;
	sched_list_item_conf->tx.phy_lbt_rssi_threshold_max =
		current_settings->rssi_scan.busy_threshold;

	sched_list_item_conf->tx.harq_feedback_requested = false;

	sched_list_item->sched_config.tx.encoded_payload_pdu_size = encoded_pdu_length;
	memcpy(sched_list_item->sched_config.tx.encoded_payload_pdu, encoded_data_to_send,
	       sched_list_item->sched_config.tx.encoded_payload_pdu_size);

	sched_list_item->sched_config.tx.header_type = DECT_PHY_HEADER_TYPE2;
	memcpy(&sched_list_item->sched_config.tx.phy_header.type_2, &phy_header.type_2,
	       sizeof(phy_header.type_2));

	sched_list_item->priority = DECT_PRIORITY1_TX;
	sched_list_item->phy_op_handle = DECT_PHY_MAC_CLIENT_ASSOCIATION_REL_TX_HANDLE;

	/* Add tx operation to scheduler list */
	if (!dect_phy_api_scheduler_list_item_add(sched_list_item)) {
		desh_error("(%s): dect_phy_api_scheduler_list_item_add failed", (__func__));
		dect_phy_api_scheduler_list_item_dealloc(sched_list_item);
		return -EBUSY;
	}

	desh_print("Scheduled random access data TX/RX:\n"
		   "  target long rd id %u (0x%08x), short rd id %u (0x%04x),\n"
		   "  target 32bit nw id %u (0x%08x), tx pwr %d dbm,\n"
		   "  channel %d, payload PDU byte count: %d,\n"
		   "  beacon interval %d, frame time %lld, beacon received %lld",
		   params->target_long_rd_id, params->target_long_rd_id, target_nbr->short_rd_id,
		   target_nbr->short_rd_id, target_nbr->nw_id_32bit, target_nbr->nw_id_32bit,
		   params->tx_power_dbm, target_nbr->channel, encoded_pdu_length,
		   beacon_interval_ms, sched_list_item_conf->frame_time, beacon_received);

	return 0;
}

int dect_phy_mac_client_dissociate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				   struct dect_phy_mac_associate_params *params)
{
	struct dect_phy_mac_client_association_data *association_data =
		dect_phy_mac_client_association_data_get(target_nbr->long_rd_id);
	int err;

	if (!association_data) {
		desh_warn("(%s): Not associated with device %d",
			(__func__), target_nbr->long_rd_id);
		return -EINVAL;
	}

	k_work_cancel_delayable(&association_data->association_resp_wait_work);

	dect_phy_mac_nbr_bg_scan_stop(association_data->bg_scan_phy_handle);
	association_data->bg_scan_ongoing = false;

	association_data->state = DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_DISASSOCIATED;
	association_data->target_nbr = NULL;
	association_data->target_long_rd_id = 0;

	err = dect_phy_mac_client_dissociate_msg_send(target_nbr, params);
	if (err) {
		desh_error("(%s): dect_phy_mac_client_associate_msg_send failed: %d",
			(__func__), err);
	}

	return err;
}

bool dect_phy_mac_client_associated_by_target_short_rd_id(uint16_t target_short_rd_id)
{
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (client_data.associations[i].state ==
			    DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_ASSOCIATED ||
		    client_data.associations[i].state ==
			    DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_WAITING_ASSOCIATION_RESP) {
			if (client_data.associations[i].target_nbr->short_rd_id ==
			    target_short_rd_id) {
				return true;
			}
		}
	}
	return false;
}

void dect_phy_mac_client_status_print(void)
{
	desh_print("Client status:");
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (client_data.associations[i].state !=
		    DECT_PHY_MAC_CLIENT_ASSOCIATION_STATE_DISASSOCIATED) {
			desh_print("  Association #%d: long RD ID %u",
				i + 1, client_data.associations[i].target_long_rd_id);
		}
	}
}

static int dect_phy_mac_client_init(void)
{
	k_work_init_delayable(&client_rach_tx_work_data.work, dect_phy_mac_client_rach_tx_worker);
	dect_phy_mac_client_association_data_init();

	return 0;
}

SYS_INIT(dect_phy_mac_client_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
