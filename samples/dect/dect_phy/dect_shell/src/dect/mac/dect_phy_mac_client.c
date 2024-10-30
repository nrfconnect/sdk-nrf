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
#include "desh_print.h"
#include "nrf_modem_dect_phy.h"

#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_common_pdu.h"
#include "dect_common_settings.h"

#include "dect_phy_api_scheduler.h"

#include "dect_phy_shell.h"
#include "dect_phy_mac_pdu.h"
#include "dect_phy_mac_nbr.h"
#include "dect_phy_mac.h"
#include "dect_phy_mac_client.h"

/**************************************************************************************************/

static struct dect_phy_mac_ctrl_data {
	uint16_t client_seq_nbr;

	uint64_t last_tx_time_mdm_ticks;

} client_data = {
	.client_seq_nbr = 0,
	.last_tx_time_mdm_ticks = 0,
};

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
	uint64_t first_possible_tx =
		time_now + (US_TO_MODEM_TICKS(current_settings->scheduler.scheduling_delay_us));
	uint64_t next_beacon_frame_start, beacon_interval_mdm_ticks, ra_start_mdm_ticks;
	uint64_t ra_interval_mdm_ticks, last_valid_rach_rx_frame_time;

	int32_t beacon_interval_ms = dect_phy_mac_pdu_cluster_beacon_period_in_ms(
		target_nbr->beacon_msg.cluster_beacon_period);

	if (beacon_interval_ms < 0) {
		return 0;
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
	/* ...and in 1st RA if ok. Additionally, to get RX really on target:
	 * delay TX by 2 subslots.
	 */
	ra_start_mdm_ticks = next_beacon_frame_start + ((target_nbr->ra_ie.start_subslot + 2) *
							DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS);
	while (ra_start_mdm_ticks < first_possible_tx &&
	       ra_start_mdm_ticks < last_valid_rach_rx_frame_time) {
		ra_start_mdm_ticks += ra_interval_mdm_ticks;
	}

	return ra_start_mdm_ticks;
}

int dect_phy_mac_client_rach_tx(struct dect_phy_mac_nbr_info_list_item *target_nbr,
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

	sched_list_item->priority = DECT_PRIORITY1_TX;
	sched_list_item->phy_op_handle = DECT_PHY_MAC_CLIENT_RA_TX_HANDLE;

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

int dect_phy_mac_client_associate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
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

	sched_list_item->priority = DECT_PRIORITY1_TX;
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

	sched_list_item->priority = DECT_PRIORITY1_RX;
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

int dect_phy_mac_client_dissociate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
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
