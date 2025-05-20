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

#include "desh_print.h"
#include "dect_common_utils.h"
#include "dect_common_pdu.h"
#include "dect_phy_ping_pdu.h"

BUILD_ASSERT((DECT_DATA_MAX_LEN == 700 && sizeof(dect_phy_ping_pdu_t) == DECT_DATA_MAX_LEN),
	     "dect_phy_perf_pdu_t must be 700 bytes");

/**************************************************************************************************/

uint8_t *dect_phy_ping_pdu_encode(uint8_t *p_target, const dect_phy_ping_pdu_t *p_input)
{
	/* To differentiate with normal DECT NR+ MAC type header, we need to
	 * set MSB up for version bits
	 */
	*p_target++ = ((1 & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 7) +
		      (p_input->header.message_type & DECT_COMMON_UTILS_BIT_MASK_7BIT);
	*p_target++ = p_input->header.pwr_ctrl_expected_rssi_level_dbm;
	p_target = dect_common_utils_16bit_be_write(p_target, p_input->header.transmitter_id);

	if (p_input->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_REQUEST ||
	    p_input->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESPONSE) {
		p_target = dect_common_utils_16bit_be_write(p_target,
							    p_input->message.tx_data.seq_nbr);
		p_target = dect_common_utils_16bit_be_write(
			p_target,
			p_input->message.tx_data.payload_length);

		memcpy(p_target, p_input->message.tx_data.pdu_payload,
		       p_input->message.tx_data.payload_length);
		p_target = p_target + sizeof(p_input->message.tx_data.pdu_payload);
	} else if (p_input->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ) {
		p_target = dect_common_utils_32bit_be_write(p_target,
							    p_input->message.results_req.unused);
	} else if (p_input->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP) {
		strncpy(p_target, p_input->message.results.results_str,
			DECT_PHY_PING_RESULTS_DATA_MAX_LEN - 1);
	}

	return p_target;
}

int dect_phy_ping_pdu_decode(dect_phy_ping_pdu_t *p_target, const uint8_t *p_data)
{
	const uint8_t *p_ptr = p_data;

	p_target->header.message_type = *p_ptr++ & DECT_COMMON_UTILS_BIT_MASK_7BIT;
	p_target->header.pwr_ctrl_expected_rssi_level_dbm = *p_ptr++;
	p_target->header.transmitter_id = dect_common_utils_16bit_be_read(&p_ptr);

	if (p_target->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_REQUEST ||
	    p_target->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESPONSE) {
		p_target->message.tx_data.seq_nbr = dect_common_utils_16bit_be_read(&p_ptr);
		p_target->message.tx_data.payload_length = dect_common_utils_16bit_be_read(&p_ptr);
		memcpy(p_target->message.tx_data.pdu_payload, p_ptr,
		       p_target->message.tx_data.payload_length);
	} else if (p_target->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP) {
		strncpy(p_target->message.results.results_str, p_ptr,
			DECT_PHY_PING_RESULTS_DATA_MAX_LEN - 1);
	}

	return 0;
}

void dect_phy_pdu_utils_ping_print(dect_phy_ping_pdu_t *ping_pdu)
{
	char snum[64] = {0};

	desh_print("  Type:                 %s",
		   dect_common_pdu_message_type_to_string(ping_pdu->header.message_type, snum));
	desh_print("  Transmitter ID:       %d", ping_pdu->header.transmitter_id);
	desh_print("  Power control:");
	desh_print("    Expected RX RSSI level (dBm): %d",
		   ping_pdu->header.pwr_ctrl_expected_rssi_level_dbm);
	if (ping_pdu->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_REQUEST ||
	    ping_pdu->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESPONSE) {
		char ascii_data[DECT_PHY_PING_TX_DATA_PDU_PAYLOAD_MAX_LEN + 1];
		int i;

		desh_print("  Seq nbr:              %d", ping_pdu->message.tx_data.seq_nbr);
		desh_print("  Payload len:          %d", ping_pdu->message.tx_data.payload_length);

		__ASSERT_NO_MSG(ping_pdu->message.tx_data.payload_length <=
				DECT_PHY_PING_TX_DATA_PDU_PAYLOAD_MAX_LEN);

		for (i = 0; i < ping_pdu->message.tx_data.payload_length; i++) {
			sprintf(&ascii_data[i], "%c", ping_pdu->message.tx_data.pdu_payload[i]);
		}
		ascii_data[i + 1] = '\0';
		desh_print("  payload data: %s", ascii_data);
	} else if (ping_pdu->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ) {

	} else if (ping_pdu->header.message_type == DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP) {
		desh_print("  results data: %s", ping_pdu->message.results.results_str);
	}
}

/**************************************************************************************************/
