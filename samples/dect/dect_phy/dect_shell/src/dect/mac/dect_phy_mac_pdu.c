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
#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_common_pdu.h"

#include "dect_phy_mac_pdu.h"
#include "dect_phy_mac.h"

/**************************************************************************************************/

const char *dect_phy_mac_pdu_header_type_to_string(int type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{DECT_PHY_MAC_HEADER_TYPE_PDU, "DATA MAC PDU header"},
		{DECT_PHY_MAC_HEADER_TYPE_BEACON, "Beacon Header"},
		{DECT_PHY_MAC_HEADER_TYPE_UNICAST, "Unicast Header"},
		{DECT_PHY_MAC_HEADER_TYPE_BROADCAST, "RD Broadcasting Header"},
		{DECT_PHY_MAC_HEADER_ESCAPE, "Escape"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, type, out_str_buff);
}

const char *dect_phy_mac_pdu_security_to_string(int type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{DECT_PHY_MAC_SECURITY_NOT_USED, "MAC security is not used"},
		{DECT_PHY_MAC_SECURITY_USED, "MAC security, no IE"},
		{DECT_PHY_MAC_SECURITY_USED_WITH_IE, "MAC security with IE"},
		{DECT_PHY_MAC_HEADER_TYPE_BROADCAST, "RD Broadcasting Header"},
		{DECT_PHY_MAC_SECURITY_RESERVED, "Reserved"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, type, out_str_buff);
}

/**************************************************************************************************/

bool dect_phy_mac_pdu_header_type_value_valid(uint8_t type_value)
{
	bool valid;

	switch (type_value) {
	case DECT_PHY_MAC_HEADER_TYPE_PDU:
	case DECT_PHY_MAC_HEADER_TYPE_BEACON:
	case DECT_PHY_MAC_HEADER_TYPE_UNICAST:
	case DECT_PHY_MAC_HEADER_TYPE_BROADCAST:
	case DECT_PHY_MAC_HEADER_ESCAPE:
		valid = true;
		break;

	default:
		valid = false;
		break;
	}

	return valid;
}

bool dect_phy_mac_pdu_type_header_decode(uint8_t *header_ptr, uint32_t data_len,
					 dect_phy_mac_type_header_t *type_header)
{
	if (data_len == 0) {
		return false;
	}
	uint8_t type;

	/* MAC spec: 6.3.2 MAC Header type */
	type_header->version = (*header_ptr >> 6) & DECT_COMMON_UTILS_BIT_MASK_2BIT;
	type_header->security = (*header_ptr >> 4) & DECT_COMMON_UTILS_BIT_MASK_2BIT;
	type = *header_ptr & DECT_COMMON_UTILS_BIT_MASK_4BIT;

	/* Validate header so far  */
	if (type_header->version != 0 ||
	    type_header->security > DECT_PHY_MAC_SECURITY_USED_WITH_IE ||
	    !dect_phy_mac_pdu_header_type_value_valid(type)) {
		return false;
	}
	type_header->type = type;

	return true;
}

uint8_t *dect_phy_mac_pdu_type_header_encode(dect_phy_mac_type_header_t *type_header_in,
					     uint8_t *target_ptr)
{
	*target_ptr++ = ((type_header_in->version & DECT_COMMON_UTILS_BIT_MASK_2BIT) << 6) +
			((type_header_in->security & DECT_COMMON_UTILS_BIT_MASK_2BIT) << 4) +
			(type_header_in->type & DECT_COMMON_UTILS_BIT_MASK_4BIT);
	return target_ptr;
}

/**************************************************************************************************/

bool dect_phy_mac_pdu_ie_type_value_0byte_len_valid(uint8_t type_value)
{
	bool valid;

	switch (type_value) {
	case DECT_PHY_MAC_IE_TYPE_0BYTE_PADDING:
	case DECT_PHY_MAC_IE_TYPE_0BYTE_CONFIG_REQ_IE:
	case DECT_PHY_MAC_IE_TYPE_0BYTE_KEEP_ALIVE_IE:
	case DECT_PHY_MAC_IE_TYPE_0BYTE_SECURITY_INFO_IE:
	case DECT_PHY_MAC_IE_TYPE_0BYTE_ESCAPE:
		valid = true;
		break;

	default:
		valid = false;
		break;
	}

	return valid;
}

bool dect_phy_mac_pdu_ie_type_value_1byte_len_valid(uint8_t type_value)
{
	bool valid;

	switch (type_value) {
	case DECT_PHY_MAC_IE_TYPE_1BYTE_PADDING:
	case DECT_PHY_MAC_IE_TYPE_1BYTE_RADIO_DEV_STATUS_IE:
	case DECT_PHY_MAC_IE_TYPE_1BYTE_ESCAPE:
		valid = true;
		break;

	default:
		valid = false;
		break;
	}

	return valid;
}

bool dect_phy_mac_pdu_ie_type_value_valid(uint8_t type_value)
{
	bool valid;

	switch (type_value) {
	case DECT_PHY_MAC_IE_TYPE_PADDING:
	case DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW1:
	case DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW2:
	case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW1:
	case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW2:
	case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW3:
	case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW4:
	case DECT_PHY_MAC_IE_TYPE_NETWORK_BEACON:
	case DECT_PHY_MAC_IE_TYPE_CLUSTER_BEACON:
	case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REQ:
	case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_RESP:
	case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REL:
	case DECT_PHY_MAC_IE_TYPE_RECONFIG_REQ:
	case DECT_PHY_MAC_IE_TYPE_RECONFIG_RESP:
	case DECT_PHY_MAC_IE_TYPE_ADDITIONAL_MAC_MESSAGES:
	case DECT_PHY_MAC_IE_TYPE_SECURITY_INFO_IE:
	case DECT_PHY_MAC_IE_TYPE_ROUTE_INFO_IE:
	case DECT_PHY_MAC_IE_TYPE_RESOURCE_ALLOCATION_IE:
	case DECT_PHY_MAC_IE_TYPE_RANDOM_ACCESS_RESOURCE_IE:
	case DECT_PHY_MAC_IE_TYPE_RD_CAPABILITY_IE:
	case DECT_PHY_MAC_IE_TYPE_NEIGHBOURING_IE:
	case DECT_PHY_MAC_IE_TYPE_BROADCAST_IND_IE:
	case DECT_PHY_MAC_IE_TYPE_GROUP_ASSIGNMENT_IE:
	case DECT_PHY_MAC_IE_TYPE_LOAD_INFO_IE:
	case DECT_PHY_MAC_IE_TYPE_MEASUREMENT_REPORT:
	case DECT_PHY_MAC_IE_TYPE_ESCAPE:
	case DECT_PHY_MAC_IE_TYPE_EXTENSION:
		valid = true;
		break;

	default:
		valid = false;
		break;
	}

	return valid;
}

const char *dect_phy_mac_pdu_ie_type_to_string(dect_phy_mac_ext_field_t mac_ext, int len, int type,
					       char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table1[] = {
		{DECT_PHY_MAC_IE_TYPE_PADDING, "Padding"},
		{DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW1,
		 "Higher layer signalling - flow 1"},
		{DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW2,
		 "Higher layer signalling - flow 2"},
		{DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW1, "User plane data - flow 1"},
		{DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW2, "User plane data - flow 2"},
		{DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW3, "User plane data - flow 3"},
		{DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW4, "User plane data - flow 4"},
		{DECT_PHY_MAC_IE_TYPE_NETWORK_BEACON, "Network Beacon message"},
		{DECT_PHY_MAC_IE_TYPE_CLUSTER_BEACON, "Cluster Beacon message"},
		{DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REQ, "Association Request message"},
		{DECT_PHY_MAC_IE_TYPE_ASSOCIATION_RESP, "Association Response message"},
		{DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REL, "Association Release message"},
		{DECT_PHY_MAC_IE_TYPE_RECONFIG_REQ, "Reconfiguration Request message"},
		{DECT_PHY_MAC_IE_TYPE_RECONFIG_RESP, "Reconfiguration Response message"},
		{DECT_PHY_MAC_IE_TYPE_ADDITIONAL_MAC_MESSAGES, "Additional MAC messages"},
		{DECT_PHY_MAC_IE_TYPE_SECURITY_INFO_IE, "MAC Security Info IE"},
		{DECT_PHY_MAC_IE_TYPE_ROUTE_INFO_IE, "Route Info IE"},
		{DECT_PHY_MAC_IE_TYPE_RESOURCE_ALLOCATION_IE, "Resource allocation IE"},
		{DECT_PHY_MAC_IE_TYPE_RANDOM_ACCESS_RESOURCE_IE, "Random Access Resource IE"},
		{DECT_PHY_MAC_IE_TYPE_RD_CAPABILITY_IE, "RD capability IE"},
		{DECT_PHY_MAC_IE_TYPE_NEIGHBOURING_IE, "Neighbouring IE"},
		{DECT_PHY_MAC_IE_TYPE_BROADCAST_IND_IE, "Broadcast Indication IE"},
		{DECT_PHY_MAC_IE_TYPE_GROUP_ASSIGNMENT_IE, "Group Assignment IE"},
		{DECT_PHY_MAC_IE_TYPE_LOAD_INFO_IE, "Load Info IE"},
		{DECT_PHY_MAC_IE_TYPE_ESCAPE, "Escape"},
		{DECT_PHY_MAC_IE_TYPE_EXTENSION, "IE type extension"},
		{-1, NULL}};
	struct mapping_tbl_item const mapping_table2[] = {
		{DECT_PHY_MAC_IE_TYPE_0BYTE_PADDING, "Padding (0 byte)"},
		{DECT_PHY_MAC_IE_TYPE_0BYTE_CONFIG_REQ_IE, "Configuration Request IE"},
		{DECT_PHY_MAC_IE_TYPE_0BYTE_KEEP_ALIVE_IE, "Keep alive IE"},
		{DECT_PHY_MAC_IE_TYPE_0BYTE_SECURITY_INFO_IE, "MAC Security Info IE"},
		{DECT_PHY_MAC_IE_TYPE_0BYTE_ESCAPE, "Escape"},
		{-1, NULL}};
	struct mapping_tbl_item const mapping_table3[] = {
		{DECT_PHY_MAC_IE_TYPE_1BYTE_PADDING, "Padding (1 byte)"},
		{DECT_PHY_MAC_IE_TYPE_1BYTE_RADIO_DEV_STATUS_IE, "Radio Device Status IE"},
		{DECT_PHY_MAC_IE_TYPE_1BYTE_ESCAPE, "Escape"},
		{-1, NULL}};

	if (mac_ext == DECT_PHY_MAC_EXT_SHORT_IE) {
		if (len == 0) {
			return dect_common_utils_map_to_string(mapping_table2, type, out_str_buff);
		} else {
			return dect_common_utils_map_to_string(mapping_table3, type, out_str_buff);
		}
	} else {
		return dect_common_utils_map_to_string(mapping_table1, type, out_str_buff);
	}
}

/**************************************************************************************************/

uint8_t dect_phy_mac_pdy_type_n_common_header_len_get(dect_phy_mac_header_type_t type)
{
	uint8_t len;

	switch (type) {
	case DECT_PHY_MAC_HEADER_TYPE_PDU:
		len = DECT_PHY_MAC_PDU_HEADER_SIZE;
		break;
	case DECT_PHY_MAC_HEADER_TYPE_BEACON:
		len = DECT_PHY_MAC_BEACON_HEADER_SIZE;
		break;

	case DECT_PHY_MAC_HEADER_TYPE_UNICAST:
		len = DECT_PHY_MAC_UNICAST_HEADER_SIZE;
		break;

	case DECT_PHY_MAC_HEADER_TYPE_BROADCAST:
		len = DECT_PHY_MAC_BROADCAST_HEADER_SIZE;
		break;

	default:
		len = 0;
		break;
	}

	return len;
}

bool dect_phy_mac_pdu_common_header_decode(const uint8_t *header_ptr, uint32_t data_len,
					   dect_phy_mac_type_header_t *type_header_in,
					   dect_phy_mac_common_header_t *common_header_out)
{
	if (data_len < 2) {
		return false;
	}
	const uint8_t *p_ptr = header_ptr;
	uint8_t header_size = 2;

	common_header_out->type = type_header_in->type;
	if (type_header_in->type == DECT_PHY_MAC_HEADER_TYPE_BEACON) {
		header_size = 7;
		if (header_size > data_len) {
			return false;
		}
		common_header_out->nw_id = dect_common_utils_24bit_be_read(&p_ptr);
		common_header_out->transmitter_id = dect_common_utils_32bit_be_read(&p_ptr);
	} else {
		uint16_t common_header_value = dect_common_utils_16bit_be_read(&p_ptr);

		common_header_out->reset =
			(common_header_value >> 12) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
		common_header_out->seq_nbr = common_header_value & DECT_COMMON_UTILS_BIT_MASK_12BIT;
		if (type_header_in->type == DECT_PHY_MAC_HEADER_TYPE_UNICAST) {
			header_size += 8;
			if (header_size > data_len) {
				return false;
			}
			common_header_out->receiver_id = dect_common_utils_32bit_be_read(&p_ptr);
			common_header_out->transmitter_id = dect_common_utils_32bit_be_read(&p_ptr);
		} else if (type_header_in->type == DECT_PHY_MAC_HEADER_TYPE_BROADCAST) {
			header_size += 4;
			if (header_size > data_len) {
				return false;
			}
			common_header_out->transmitter_id = dect_common_utils_32bit_be_read(&p_ptr);
		}
	}

	return true;
}

uint8_t *dect_phy_mac_pdu_common_header_encode(dect_phy_mac_common_header_t *common_header_in,
					       uint8_t *target_ptr)
{
	if (common_header_in->type == DECT_PHY_MAC_HEADER_TYPE_BEACON) {
		target_ptr = dect_common_utils_24bit_be_write(target_ptr, common_header_in->nw_id);
		target_ptr = dect_common_utils_32bit_be_write(target_ptr,
							      common_header_in->transmitter_id);
	} else {
		uint16_t value =
			((common_header_in->reset & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 12) +
			(common_header_in->seq_nbr & DECT_COMMON_UTILS_BIT_MASK_12BIT);

		target_ptr = dect_common_utils_16bit_be_write(target_ptr, value);
		if (common_header_in->type == DECT_PHY_MAC_HEADER_TYPE_UNICAST) {
			target_ptr = dect_common_utils_32bit_be_write(
				target_ptr, common_header_in->receiver_id);
			target_ptr = dect_common_utils_32bit_be_write(
				target_ptr, common_header_in->transmitter_id);
		} else if (common_header_in->type == DECT_PHY_MAC_HEADER_TYPE_BROADCAST) {
			target_ptr = dect_common_utils_32bit_be_write(
				target_ptr, common_header_in->transmitter_id);
		}
	}

	return target_ptr;
}

/**************************************************************************************************/

uint8_t dect_phy_mac_pdu_fixed_size_ie_length_get(uint8_t ie_type)
{
	uint8_t length;

	switch (ie_type) {
	case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REL:
		length = 1;
		break;
	case DECT_PHY_MAC_IE_TYPE_SECURITY_INFO_IE:
		length = 5;
		break;
	case DECT_PHY_MAC_IE_TYPE_ROUTE_INFO_IE:
		length = 5;
		break;

	default:
		length = 0;
		break;
	}

	return length;
}

/**************************************************************************************************/

bool dect_phy_mac_pdu_mux_header_decode(uint8_t *header_ptr, uint32_t data_len,
					dect_phy_mac_mux_header_t *mux_header_out)
{
	if (data_len < 1) {
		return false;
	}

	const uint8_t *p_ptr = header_ptr;
	uint8_t mac_ext = (*p_ptr >> 6) & DECT_COMMON_UTILS_BIT_MASK_2BIT;
	uint8_t ie_type;
	uint8_t header_size = 1;

	if (mac_ext == DECT_PHY_MAC_EXT_SHORT_IE) {
		/* MAC spec: Figure 6.3.4-1: Option a & b (DECT_PHY_MAC_EXT_SHORT_IE) */
		mux_header_out->mac_ext = mac_ext;
		mux_header_out->payload_length = *p_ptr >> 5 & DECT_COMMON_UTILS_BIT_MASK_1BIT;
		ie_type = *p_ptr & DECT_COMMON_UTILS_BIT_MASK_5BIT;

		if (mux_header_out->payload_length == 0) {
			/* Only values from MAC spec Table 6.3.4-3 are accepted */
			if (!dect_phy_mac_pdu_ie_type_value_0byte_len_valid(ie_type)) {
				return false;
			}
			mux_header_out->payload_ptr = NULL;
		} else {
			header_size = 2;

			/* Only values from MAC spec Table 6.3.4-4 are accepted */
			if (!dect_phy_mac_pdu_ie_type_value_1byte_len_valid(ie_type)) {
				return false;
			}
			if (header_size > data_len) {
				return false;
			}
			p_ptr++;
			mux_header_out->payload_ptr = p_ptr;
		}
		mux_header_out->ie_type = ie_type;
		return true;
	}
	/* MAC spec: Figure 6.3.4-1: Options c-f */

	/* MAC spec, Table 6.3.4-2 */
	mux_header_out->ie_type = *p_ptr++ & DECT_COMMON_UTILS_BIT_MASK_6BIT;

	if (!dect_phy_mac_pdu_ie_type_value_valid(mux_header_out->ie_type)) {
		return false;
	}

	if (mac_ext == DECT_PHY_MAC_EXT_NO_LENGTH) {
		/* Option 'c': fixed size MAC SDU */
		if (mux_header_out->ie_type == DECT_PHY_MAC_IE_TYPE_EXTENSION) {
			return false;
		}
		mux_header_out->payload_length =
			dect_phy_mac_pdu_fixed_size_ie_length_get(mux_header_out->ie_type);
		mux_header_out->payload_ptr = p_ptr;
		mux_header_out->mac_ext = mac_ext;
	} else if (mac_ext == DECT_PHY_MAC_EXT_8BIT_LEN) {
		/* Option 'd': variable size MAC SDU with 8bit length */
		if (data_len < 2) {
			return false;
		}
		header_size = 2;
		mux_header_out->payload_length = *p_ptr++;
		mux_header_out->payload_ptr = p_ptr;
		mux_header_out->mac_ext = mac_ext;
	} else if (mac_ext == DECT_PHY_MAC_EXT_16BIT_LEN) {
		/* Option 'e' and 'f': variable size MAC SDU with 16bit length */
		if (data_len < 3) {
			return false;
		}
		header_size = 2;
		mux_header_out->ie_ext = 0;
		mux_header_out->payload_length = dect_common_utils_16bit_be_read(&p_ptr);
		if (mux_header_out->ie_type == DECT_PHY_MAC_IE_TYPE_EXTENSION) {
			mux_header_out->ie_ext = *p_ptr++;
		}
		mux_header_out->payload_ptr = p_ptr;
		mux_header_out->mac_ext = mac_ext;
	} else {
		/* Unknown mac extension */
		return false;
	}
	/* Final check that payload fits to data_len excluding header_size */
	if (mux_header_out->payload_length > data_len - header_size) {
		return false;
	}

	return true;
}

uint8_t *dect_phy_mac_mux_header_header_encode(dect_phy_mac_mux_header_t *mux_header_in,
					       uint8_t *target_ptr)
{
	*target_ptr = (mux_header_in->mac_ext & DECT_COMMON_UTILS_BIT_MASK_2BIT)
		      << 6; /* Always 2 bit */
	if (mux_header_in->mac_ext == DECT_PHY_MAC_EXT_SHORT_IE) {
		uint8_t len_bit = (mux_header_in->payload_length) ? 1 : 0;

		*target_ptr++ |= ((len_bit << 5) |
				  (mux_header_in->ie_type & DECT_COMMON_UTILS_BIT_MASK_2BIT));
	} else {
		if (mux_header_in->mac_ext == DECT_PHY_MAC_EXT_NO_LENGTH) {
			return target_ptr;
		}
		*target_ptr++ |= (mux_header_in->ie_type & DECT_COMMON_UTILS_BIT_MASK_6BIT);
		if (mux_header_in->mac_ext == DECT_PHY_MAC_EXT_8BIT_LEN) {
			*target_ptr++ =
				mux_header_in->payload_length & DECT_COMMON_UTILS_BIT_MASK_8BIT;
		} else {
			__ASSERT_NO_MSG(mux_header_in->mac_ext == DECT_PHY_MAC_EXT_16BIT_LEN);

			target_ptr = dect_common_utils_16bit_be_write(
				target_ptr,
				mux_header_in->payload_length);
			if (mux_header_in->ie_type == DECT_PHY_MAC_IE_TYPE_EXTENSION) {
				*target_ptr++ = mux_header_in->ie_ext;
			}
		}
	}

	return target_ptr;
}

/* MAC spec: Figure 6.3.4-1: MAC multiplexing PDU with different header options */
uint8_t dect_phy_mac_pdu_mux_header_length_get(dect_phy_mac_mux_header_t *header_ptr)
{
	uint8_t length = 0;

	if (header_ptr->mac_ext == DECT_PHY_MAC_EXT_SHORT_IE ||
	    header_ptr->mac_ext == DECT_PHY_MAC_EXT_NO_LENGTH) {
		length = 1;
	} else {
		length = 2;
		if (header_ptr->mac_ext == DECT_PHY_MAC_EXT_16BIT_LEN) {
			length++;
		}
		if (header_ptr->ie_type == DECT_PHY_MAC_IE_TYPE_EXTENSION) {
			length++;
		}
	}
	return length;
}

static uint16_t dect_phy_mac_pdu_sdu_cluster_beacon_length_get(
	dect_phy_mac_cluster_beacon_t *beacon_msg)
{
	uint16_t length = DECT_PHY_MAC_CLUSTER_BEACON_MIN_LEN;

	if (beacon_msg->tx_pwr_bit) {
		length++;
	}
	if (beacon_msg->frame_offset_bit) {
		length++;
	}
	if (beacon_msg->next_channel_bit) {
		length += 2;
	}
	if (beacon_msg->time_to_next_next) {
		length += 4;
	}

	return length;
}

bool dect_phy_mac_pdu_sdu_cluster_beacon_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						dect_phy_mac_cluster_beacon_t *cluster_beacon_out)
{
	/* Decode the cluster beacon fields from the payload
	 * and populate the cluster_beacon_out structure
	 */
	if (payload_len < DECT_PHY_MAC_CLUSTER_BEACON_MIN_LEN) {
		return false;
	}
	uint8_t msg_len;

	cluster_beacon_out->system_frame_number = *payload_ptr++;

	cluster_beacon_out->tx_pwr_bit = (*payload_ptr >> 4) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	cluster_beacon_out->pwr_const_bit = (*payload_ptr >> 3) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	cluster_beacon_out->frame_offset_bit =
		(*payload_ptr >> 2) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	cluster_beacon_out->next_channel_bit =
		(*payload_ptr >> 1) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	cluster_beacon_out->time_to_next_next = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_1BIT;

	cluster_beacon_out->nw_beacon_period =
		(dect_phy_mac_nw_beacon_period_t)((*payload_ptr >> 4) &
						  DECT_COMMON_UTILS_BIT_MASK_4BIT);
	cluster_beacon_out->cluster_beacon_period =
		(dect_phy_mac_cluster_beacon_period_t)(*payload_ptr++ &
						       DECT_COMMON_UTILS_BIT_MASK_4BIT);

	cluster_beacon_out->count_to_trigger =
		(*payload_ptr >> 4) & DECT_COMMON_UTILS_BIT_MASK_4BIT;
	cluster_beacon_out->relative_quality =
		(*payload_ptr >> 2) & DECT_COMMON_UTILS_BIT_MASK_2BIT;
	cluster_beacon_out->min_quality = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_2BIT;

	msg_len = dect_phy_mac_pdu_sdu_cluster_beacon_length_get(cluster_beacon_out);
	if (msg_len > payload_len) {
		return false;
	}

	if (cluster_beacon_out->tx_pwr_bit) {
		cluster_beacon_out->max_phy_tx_power =
			*payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_4BIT;
	}
	if (cluster_beacon_out->frame_offset_bit) {
		cluster_beacon_out->frame_offset = *payload_ptr++;
	}
	if (cluster_beacon_out->next_channel_bit) {
		cluster_beacon_out->next_cluster_channel =
			((payload_ptr[0] & DECT_COMMON_UTILS_BIT_MASK_5BIT) << 8) | payload_ptr[1];
		payload_ptr += 2;
	}
	if (cluster_beacon_out->time_to_next_next) {
		cluster_beacon_out->time_to_next = dect_common_utils_32bit_be_read(&payload_ptr);
	}

	return true;
}

uint8_t *
dect_phy_mac_pdu_sdu_cluster_beacon_encode(const dect_phy_mac_cluster_beacon_t *cluster_beacon_in,
					   uint8_t *target_ptr)
{
	*target_ptr++ = cluster_beacon_in->system_frame_number;
	*target_ptr++ =
		((cluster_beacon_in->tx_pwr_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 4) +
		((cluster_beacon_in->pwr_const_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 3) +
		((cluster_beacon_in->frame_offset_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 2) +
		((cluster_beacon_in->next_channel_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 1) +
		(cluster_beacon_in->time_to_next_next & DECT_COMMON_UTILS_BIT_MASK_1BIT);
	*target_ptr++ =
		((cluster_beacon_in->nw_beacon_period & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 4) +
		(cluster_beacon_in->cluster_beacon_period & DECT_COMMON_UTILS_BIT_MASK_4BIT);
	*target_ptr++ =
		((cluster_beacon_in->count_to_trigger & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 4) +
		((cluster_beacon_in->relative_quality & DECT_COMMON_UTILS_BIT_MASK_2BIT) << 2) +
		(cluster_beacon_in->min_quality & DECT_COMMON_UTILS_BIT_MASK_2BIT);

	if (cluster_beacon_in->tx_pwr_bit) {
		*target_ptr++ =
			(cluster_beacon_in->max_phy_tx_power & DECT_COMMON_UTILS_BIT_MASK_4BIT);
	}
	if (cluster_beacon_in->frame_offset_bit) {
		*target_ptr++ = cluster_beacon_in->frame_offset;
	}
	if (cluster_beacon_in->next_channel_bit) {
		*target_ptr++ = cluster_beacon_in->next_cluster_channel;
	}
	if (cluster_beacon_in->time_to_next_next) {
		target_ptr = dect_common_utils_32bit_be_write(target_ptr,
							      cluster_beacon_in->time_to_next);
	}
	return target_ptr;
}

/**************************************************************************************************/

static uint16_t dect_phy_mac_ra_resource_ie_length_get(
	const dect_phy_mac_random_access_resource_ie_t *rach_ie_in)
{
	uint16_t length = DECT_PHY_MAC_RANDOM_ACCESS_RES_IE_MIN_LEN;

	if (rach_ie_in->repeat != DECT_PHY_MAC_RA_REPEAT_TYPE_SINGLE) {
		length += 2;
	}
	if (rach_ie_in->sfn_included) {
		length++;
	}
	if (rach_ie_in->channel_included) {
		length += 2;
	}
	if (rach_ie_in->channel2_included) {
		length += 2;
	}
	return length;
}

static bool dect_phy_mac_sdu_random_access_resource_decode(
	const uint8_t *payload_ptr, uint32_t payload_len,
	dect_phy_mac_random_access_resource_ie_t *rach_ie_out)
{
	if (payload_len < DECT_PHY_MAC_RANDOM_ACCESS_RES_IE_MIN_LEN) {
		return false;
	}

	/* Note: no validity check for reserved */
	rach_ie_out->repeat =
		(dect_phy_mac_rach_repeat_type_t)((*payload_ptr >> 3) &
						  DECT_COMMON_UTILS_BIT_MASK_2BIT);

	rach_ie_out->sfn_included = (*payload_ptr >> 2) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	rach_ie_out->channel_included = (*payload_ptr >> 1) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	rach_ie_out->channel2_included = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_1BIT;

	uint16_t ie_len = dect_phy_mac_ra_resource_ie_length_get(rach_ie_out);

	if (ie_len > payload_len) {
		return false;
	}

	rach_ie_out->start_subslot = *payload_ptr++;

	rach_ie_out->length_type = (enum dect_phy_packet_length_type)(
		(*payload_ptr >> 7) & DECT_COMMON_UTILS_BIT_MASK_1BIT);
	rach_ie_out->length = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_7BIT;

	rach_ie_out->max_rach_length_type = (enum dect_phy_packet_length_type)(
		(*payload_ptr >> 7) & DECT_COMMON_UTILS_BIT_MASK_1BIT);
	rach_ie_out->max_rach_length = (*payload_ptr >> 3) & DECT_COMMON_UTILS_BIT_MASK_4BIT;
	rach_ie_out->cw_min_sig = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_3BIT;

	rach_ie_out->dect_delay = (*payload_ptr >> 7) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	rach_ie_out->response_win = (*payload_ptr >> 3) & DECT_COMMON_UTILS_BIT_MASK_4BIT;
	rach_ie_out->cw_max_sig = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_3BIT;

	if (rach_ie_out->repeat != DECT_PHY_MAC_RA_REPEAT_TYPE_SINGLE) {
		rach_ie_out->repetition = *payload_ptr++;
		rach_ie_out->validity = *payload_ptr++;
	}
	if (rach_ie_out->sfn_included) {
		rach_ie_out->system_frame_number = *payload_ptr++;
	}
	if (rach_ie_out->channel_included) {
		rach_ie_out->channel1 =
			((payload_ptr[0] & DECT_COMMON_UTILS_BIT_MASK_5BIT) << 8) | payload_ptr[1];
		payload_ptr += 2;
	}
	if (rach_ie_out->channel2_included) {
		rach_ie_out->channel2 =
			((payload_ptr[0] & DECT_COMMON_UTILS_BIT_MASK_5BIT) << 8) | payload_ptr[1];
	}
	return true;
}

static uint8_t *dect_phy_mac_pdu_sdu_random_access_resource_encode(
	const dect_phy_mac_random_access_resource_ie_t *rach_ie_in, uint8_t *target_ptr)
{
	*target_ptr++ = ((rach_ie_in->repeat & DECT_COMMON_UTILS_BIT_MASK_2BIT) << 3) +
			((rach_ie_in->sfn_included & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 2) +
			((rach_ie_in->channel_included & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 1) +
			(rach_ie_in->channel2_included & DECT_COMMON_UTILS_BIT_MASK_1BIT);
	*target_ptr++ = rach_ie_in->start_subslot;
	*target_ptr++ = ((rach_ie_in->length_type & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 7) +
			(rach_ie_in->length & DECT_COMMON_UTILS_BIT_MASK_7BIT);
	*target_ptr++ =
		((rach_ie_in->max_rach_length_type & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 7) +
		((rach_ie_in->max_rach_length & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 3) +
		(rach_ie_in->cw_min_sig & DECT_COMMON_UTILS_BIT_MASK_3BIT);
	*target_ptr++ = ((rach_ie_in->dect_delay & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 7) +
			((rach_ie_in->response_win & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 3) +
			(rach_ie_in->cw_max_sig & DECT_COMMON_UTILS_BIT_MASK_3BIT);
	if (rach_ie_in->repeat != DECT_PHY_MAC_RA_REPEAT_TYPE_SINGLE) {
		*target_ptr++ = rach_ie_in->repetition;
		*target_ptr++ = rach_ie_in->validity;
	}
	if (rach_ie_in->sfn_included) {
		*target_ptr++ = rach_ie_in->system_frame_number;
	}
	if (rach_ie_in->channel_included) {
		target_ptr = dect_common_utils_16bit_be_write(target_ptr, rach_ie_in->channel1);
	}
	if (rach_ie_in->channel2_included) {
		target_ptr = dect_common_utils_16bit_be_write(target_ptr, rach_ie_in->channel2);
	}

	return target_ptr;
}

/**************************************************************************************************/

uint8_t *dect_phy_mac_pdu_sdu_association_req_encode(const dect_phy_mac_association_req_t *req_in,
						     uint8_t *target_ptr)
{
	*target_ptr++ = ((req_in->setup_cause & DECT_COMMON_UTILS_BIT_MASK_3BIT) << 5) +
			((req_in->flow_count & DECT_COMMON_UTILS_BIT_MASK_3BIT) << 2) +
			((req_in->pwr_const_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 1) +
			(req_in->ft_mode_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT);
	*target_ptr++ = (req_in->current_cluster_channel_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT)
			<< 7;
	*target_ptr++ =
		((req_in->harq_tx_process_count & DECT_COMMON_UTILS_BIT_MASK_3BIT) << 5) +
		(req_in->max_harq_tx_retransmission_delay & DECT_COMMON_UTILS_BIT_MASK_5BIT);
	*target_ptr++ =
		((req_in->harq_rx_process_count & DECT_COMMON_UTILS_BIT_MASK_3BIT) << 5) +
		(req_in->max_harq_rx_retransmission_delay & DECT_COMMON_UTILS_BIT_MASK_5BIT);

	/* Only one flow supported */
	*target_ptr++ = (req_in->flow_id & DECT_COMMON_UTILS_BIT_MASK_6BIT);
	if (req_in->ft_mode_bit) {
		*target_ptr++ =
			((req_in->nw_beacon_period & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 4) +
			(req_in->cluster_beacon_period & DECT_COMMON_UTILS_BIT_MASK_4BIT);
		target_ptr =
			dect_common_utils_16bit_be_write(target_ptr, req_in->next_cluster_channel);
		target_ptr = dect_common_utils_32bit_be_write(target_ptr, req_in->time_to_next);
	}
	if (req_in->current_cluster_channel_bit) {
		target_ptr = dect_common_utils_16bit_be_write(target_ptr,
							      req_in->current_cluster_channel);
	}
	return target_ptr;
}

static uint16_t dect_phy_mac_pdu_sdu_association_req_length_get(
	const dect_phy_mac_association_req_t *req_in)
{
	uint16_t length = DECT_PHY_MAC_ASSOCIATION_REQ_MIN_LEN;

	if (req_in->ft_mode_bit) {
		length += 7;
	}
	if (req_in->current_cluster_channel_bit) {
		length += 2;
	}
	return length;
}

bool dect_phy_mac_pdu_sdu_association_req_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						 dect_phy_mac_association_req_t *req_out)
{
	if (payload_len < DECT_PHY_MAC_ASSOCIATION_REQ_MIN_LEN) {
		return false;
	}
	uint8_t msg_len;

	req_out->setup_cause = (*payload_ptr >> 5) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
	req_out->flow_count = (*payload_ptr >> 2) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
	req_out->pwr_const_bit = (*payload_ptr >> 1) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	req_out->ft_mode_bit = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_1BIT;

	req_out->current_cluster_channel_bit =
		(*payload_ptr++ >> 7) & DECT_COMMON_UTILS_BIT_MASK_1BIT;

	req_out->harq_tx_process_count = (*payload_ptr >> 5) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
	req_out->max_harq_tx_retransmission_delay =
		*payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_5BIT;

	req_out->harq_rx_process_count = (*payload_ptr >> 5) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
	req_out->max_harq_rx_retransmission_delay =
		*payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_5BIT;

	req_out->flow_id = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_6BIT;

	msg_len =  dect_phy_mac_pdu_sdu_association_req_length_get(req_out);
	if (msg_len > payload_len) {
		return false;
	}

	if (req_out->ft_mode_bit) {
		req_out->nw_beacon_period = (*payload_ptr >> 4) & DECT_COMMON_UTILS_BIT_MASK_4BIT;
		req_out->cluster_beacon_period = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_4BIT;
		req_out->next_cluster_channel = dect_common_utils_16bit_be_read(&payload_ptr);
		req_out->time_to_next = dect_common_utils_32bit_be_read(&payload_ptr);
	}
	if (req_out->current_cluster_channel_bit) {
		req_out->current_cluster_channel = dect_common_utils_16bit_be_read(&payload_ptr);
	}
	return true;
}

uint8_t *
dect_phy_mac_pdu_sdu_association_resp_encode(const dect_phy_mac_association_resp_t *resp_in,
					     uint8_t *target_ptr)
{
	*target_ptr++ = ((resp_in->ack_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 7) +
			((resp_in->harq_conf_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 5) +
			((resp_in->flow_count & DECT_COMMON_UTILS_BIT_MASK_3BIT) << 2) +
			((resp_in->group_bit & DECT_COMMON_UTILS_BIT_MASK_1BIT) << 1);
	if (!resp_in->ack_bit) {
		*target_ptr++ = ((resp_in->reject_cause & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 4) +
				(resp_in->reject_time & DECT_COMMON_UTILS_BIT_MASK_4BIT);
	} else {
		if (resp_in->harq_conf_bit) {
			*target_ptr++ =
				((resp_in->harq_rx_process_count & DECT_COMMON_UTILS_BIT_MASK_3BIT)
				 << 5) +
				(resp_in->max_harq_rx_retransmission_delay &
				 DECT_COMMON_UTILS_BIT_MASK_5BIT);
			*target_ptr++ =
				((resp_in->harq_tx_process_count & DECT_COMMON_UTILS_BIT_MASK_3BIT)
				 << 5) +
				(resp_in->max_harq_tx_retransmission_delay &
				 DECT_COMMON_UTILS_BIT_MASK_5BIT);
		}
		*target_ptr++ = resp_in->flow_id[0];
		if (resp_in->group_bit) {
			*target_ptr++ = resp_in->group_id & DECT_COMMON_UTILS_BIT_MASK_7BIT;
			*target_ptr++ = resp_in->resource_tag & DECT_COMMON_UTILS_BIT_MASK_7BIT;
		}
	}
	return target_ptr;
}

static uint16_t dect_phy_mac_pdu_sdu_association_resp_length_get(
	const dect_phy_mac_association_resp_t *resp)
{
	uint16_t length = DECT_PHY_MAC_ASSOCIATION_RESP_MIN_LEN;

	if (resp->harq_conf_bit) {
		length += 2;
	}
	if (resp->group_bit) {
		length += 2;
	}

	return length;
}

bool dect_phy_mac_pdu_sdu_association_resp_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						  dect_phy_mac_association_resp_t *resp_out)
{
	if (payload_len < DECT_PHY_MAC_ASSOCIATION_RESP_MIN_LEN) {
		return false;
	}
	uint8_t msg_len;

	resp_out->ack_bit = (*payload_ptr >> 7) & DECT_COMMON_UTILS_BIT_MASK_1BIT;

	if (!resp_out->ack_bit) {
		resp_out->reject_cause = (*payload_ptr >> 4) & DECT_COMMON_UTILS_BIT_MASK_4BIT;
		resp_out->reject_time = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_4BIT;

		return true;
	}
	resp_out->harq_conf_bit = (*payload_ptr >> 5) & DECT_COMMON_UTILS_BIT_MASK_1BIT;
	resp_out->flow_count = (*payload_ptr >> 2) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
	resp_out->group_bit = (*payload_ptr++ >> 1) & DECT_COMMON_UTILS_BIT_MASK_1BIT;

	msg_len = dect_phy_mac_pdu_sdu_association_resp_length_get(resp_out);
	if (msg_len > payload_len) {
		return false;
	}

	if (resp_out->harq_conf_bit) {
		resp_out->harq_rx_process_count =
			(*payload_ptr >> 5) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
		resp_out->max_harq_rx_retransmission_delay =
			*payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_5BIT;
		resp_out->harq_tx_process_count =
			(*payload_ptr >> 5) & DECT_COMMON_UTILS_BIT_MASK_3BIT;
		resp_out->max_harq_tx_retransmission_delay =
			*payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_5BIT;
	}
	resp_out->flow_id[0] = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_6BIT;
	if (resp_out->group_bit) {
		resp_out->group_id = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_7BIT;
		resp_out->resource_tag = *payload_ptr++ & DECT_COMMON_UTILS_BIT_MASK_7BIT;
	}
	return true;
}

uint8_t *dect_phy_mac_pdu_sdu_association_rel_encode(const dect_phy_mac_association_rel_t *req_in,
						     uint8_t *target_ptr)
{
	*target_ptr++ = (req_in->rel_cause & DECT_COMMON_UTILS_BIT_MASK_4BIT) << 4;

	return target_ptr;
}

bool dect_phy_mac_pdu_sdu_association_rel_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						 dect_phy_mac_association_rel_t *req_out)
{
	if (payload_len < DECT_PHY_MAC_ASSOCIATION_REL_LEN) {
		return false;
	}
	req_out->rel_cause = (*payload_ptr >> 4) & DECT_COMMON_UTILS_BIT_MASK_4BIT;

	return true;
}

/**************************************************************************************************/

uint8_t *dect_phy_mac_pdu_sdus_encode(uint8_t *target_ptr, sys_dlist_t *sdu_input_list)
{
	/* This is expecting that MAC type and common header are written outside of this function */
	dect_phy_mac_sdu_t *sdu_list_item;

	while (!sys_dlist_is_empty(sdu_input_list)) {
		/* Remove 1st item from the list and get the node */
		sys_dnode_t *node = sys_dlist_get(sdu_input_list);

		if (node != NULL) {
			sdu_list_item = CONTAINER_OF(node, dect_phy_mac_sdu_t, dnode);

			/* Encode the MUX header */
			target_ptr = dect_phy_mac_mux_header_header_encode(
				&sdu_list_item->mux_header, target_ptr);
			switch (sdu_list_item->message_type) {
			case DECT_PHY_MAC_MESSAGE_TYPE_DATA_SDU:
				__ASSERT_NO_MSG(sdu_list_item->message.data_sdu.dlc_ie_type ==
						DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING);

				/* Encode the data into DLC format. See DLC spec ch. 5.3.1 and
				 * 5.3.2.
				 * Note: only DLC Service type 0 without a routing header
				 * is supported.
				 */
				*target_ptr++ = sdu_list_item->message.data_sdu.dlc_ie_type << 4;
				memcpy(target_ptr, sdu_list_item->message.data_sdu.data,
				       sdu_list_item->message.data_sdu.data_length);
				target_ptr += sdu_list_item->message.data_sdu.data_length;
				break;
			case DECT_PHY_MAC_MESSAGE_TYPE_CLUSTER_BEACON:
				/* Encode the Cluster Beacon */
				target_ptr = dect_phy_mac_pdu_sdu_cluster_beacon_encode(
					&sdu_list_item->message.cluster_beacon, target_ptr);
				break;
			case DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REQ:
				target_ptr = dect_phy_mac_pdu_sdu_association_req_encode(
					&sdu_list_item->message.association_req, target_ptr);
				break;
			case DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_RESP:
				target_ptr = dect_phy_mac_pdu_sdu_association_resp_encode(
					&sdu_list_item->message.association_resp, target_ptr);
				break;
			case DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REL:
				target_ptr = dect_phy_mac_pdu_sdu_association_rel_encode(
					&sdu_list_item->message.association_rel, target_ptr);
				break;
			case DECT_PHY_MAC_MESSAGE_RANDOM_ACCESS_RESOURCE_IE:
				/* Encode the RACH IE */
				target_ptr = dect_phy_mac_pdu_sdu_random_access_resource_encode(
					&sdu_list_item->message.rach_ie, target_ptr);
				break;
			case DECT_PHY_MAC_MESSAGE_PADDING:
				/* Encode the padding,
				 * shall be encoded when needed to end separately
				 */
				memcpy(target_ptr, sdu_list_item->message.common_msg.data,
				       sdu_list_item->message.common_msg.data_length);
				target_ptr += sdu_list_item->message.common_msg.data_length;
				break;
			case DECT_PHY_MAC_MESSAGE_ESCAPE:
				/* Encode the escape */
				memcpy(target_ptr, sdu_list_item->message.common_msg.data,
				       sdu_list_item->message.common_msg.data_length);
				target_ptr += sdu_list_item->message.common_msg.data_length;
				break;
			default:
				/* Encode the unknown */
				memcpy(target_ptr, sdu_list_item->message.common_msg.data,
				       sdu_list_item->message.common_msg.data_length);
				target_ptr += sdu_list_item->message.common_msg.data_length;
				break;
			}

			k_free(sdu_list_item);
		}
	}
	return target_ptr;
}

bool dect_phy_mac_pdu_sdus_decode(uint8_t *payload_ptr, uint32_t payload_len, sys_dlist_t *sdu_list)
{
	uint8_t *sdu_ptr = payload_ptr;
	uint8_t *pdu_end_ptr = payload_ptr + payload_len;
	uint32_t remaining_len = payload_len;
	bool handled = false;

	while (sdu_ptr < pdu_end_ptr) {
		dect_phy_mac_mux_header_t mux_header;

		if (!dect_phy_mac_pdu_mux_header_decode(sdu_ptr, remaining_len, &mux_header)) {
			printk("Failed to decode MAC MUX header\n");
			return false;
		}

		/* Add SDU to list */
		dect_phy_mac_sdu_t *sdu_list_item =
			(dect_phy_mac_sdu_t *)k_malloc(sizeof(dect_phy_mac_sdu_t));

		if (sdu_list_item == NULL) {
			printk("No memory to decode SDUs!\n");
			return false;
		}

		sdu_list_item->mux_header = mux_header;

		switch (sdu_list_item->mux_header.ie_type) {
		/* Only supported types */
		case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW1:
		case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW2:
		case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW3:
		case DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW4:
		case DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW1:
		case DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW2: {
			uint8_t *sdu_ptr = (uint8_t *)mux_header.payload_ptr;
			uint8_t dlc_ie_type = *sdu_ptr++ >> 4; /* DLC spec: ch. 5.3.2 */

			if (dlc_ie_type != DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING) {
				printk("Unsupported DLC IE type\n");
			}
			sdu_list_item->message.data_sdu.dlc_ie_type = dlc_ie_type;
			sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_DATA_SDU;
			sdu_list_item->message.data_sdu.data_length =
				mux_header.payload_length -
				DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING_LEN;
			memcpy(sdu_list_item->message.data_sdu.data, sdu_ptr,
			       sdu_list_item->message.data_sdu.data_length);
			break;
		}

		case DECT_PHY_MAC_IE_TYPE_CLUSTER_BEACON:
			handled = dect_phy_mac_pdu_sdu_cluster_beacon_decode(
				mux_header.payload_ptr, mux_header.payload_length,
				&sdu_list_item->message.cluster_beacon);
			if (!handled) {
				printk("Failed to decode Cluster Beacon\n");
				sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_NONE;
				sdu_list_item->message.common_msg.data_length =
					mux_header.payload_length;
				memcpy(sdu_list_item->message.common_msg.data,
				       mux_header.payload_ptr, mux_header.payload_length);
			} else {
				sdu_list_item->message_type =
					DECT_PHY_MAC_MESSAGE_TYPE_CLUSTER_BEACON;
			}

			break;
		case DECT_PHY_MAC_IE_TYPE_RANDOM_ACCESS_RESOURCE_IE:
			handled = dect_phy_mac_sdu_random_access_resource_decode(
				mux_header.payload_ptr, mux_header.payload_length,
				&sdu_list_item->message.rach_ie);

			if (!handled) {
				printk("Failed to decode RACH IE\n");
				sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_NONE;
				sdu_list_item->message.common_msg.data_length =
					mux_header.payload_length;
				memcpy(sdu_list_item->message.common_msg.data,
				       mux_header.payload_ptr, mux_header.payload_length);
			} else {
				sdu_list_item->message_type =
					DECT_PHY_MAC_MESSAGE_RANDOM_ACCESS_RESOURCE_IE;
			}
			break;
		case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REQ:
			handled = dect_phy_mac_pdu_sdu_association_req_decode(
				mux_header.payload_ptr, mux_header.payload_length,
				&sdu_list_item->message.association_req);
			if (!handled) {
				printk("Failed to decode Association Request message\n");
				sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_NONE;
				sdu_list_item->message.common_msg.data_length =
					mux_header.payload_length;
				memcpy(sdu_list_item->message.common_msg.data,
				       mux_header.payload_ptr, mux_header.payload_length);
			} else {
				sdu_list_item->message_type =
					DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REQ;
			}
			break;
		case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_RESP:
			handled = dect_phy_mac_pdu_sdu_association_resp_decode(
				mux_header.payload_ptr, mux_header.payload_length,
				&sdu_list_item->message.association_resp);
			if (!handled) {
				printk("Failed to decode Association Response message\n");
				sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_NONE;
				sdu_list_item->message.common_msg.data_length =
					mux_header.payload_length;
				memcpy(sdu_list_item->message.common_msg.data,
				       mux_header.payload_ptr, mux_header.payload_length);
			} else {
				sdu_list_item->message_type =
					DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_RESP;
			}
			break;
		case DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REL:
			handled = dect_phy_mac_pdu_sdu_association_rel_decode(
				mux_header.payload_ptr, mux_header.payload_length,
				&sdu_list_item->message.association_rel);
			if (!handled) {
				printk("Failed to decode Association Release message\n");
				sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_NONE;
				sdu_list_item->message.common_msg.data_length =
					mux_header.payload_length;
				memcpy(sdu_list_item->message.common_msg.data,
				       mux_header.payload_ptr, mux_header.payload_length);
			} else {
				sdu_list_item->message_type =
					DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REL;
			}
			break;
		case DECT_PHY_MAC_IE_TYPE_PADDING:
			sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_PADDING;
			sdu_list_item->message.common_msg.data_length = mux_header.payload_length;
			memcpy(sdu_list_item->message.common_msg.data, mux_header.payload_ptr,
			       mux_header.payload_length);
			break;
		case DECT_PHY_MAC_IE_TYPE_ESCAPE:
			sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_ESCAPE;
			sdu_list_item->message.common_msg.data_length = mux_header.payload_length;
			memcpy(sdu_list_item->message.common_msg.data, mux_header.payload_ptr,
			       mux_header.payload_length);
			break;
		default:
			sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_TYPE_NONE;
			sdu_list_item->message.common_msg.data_length = mux_header.payload_length;
			memcpy(sdu_list_item->message.common_msg.data, mux_header.payload_ptr,
			       mux_header.payload_length);
			break;
		}

		sys_dlist_append(sdu_list, &sdu_list_item->dnode);

		sdu_ptr += dect_phy_mac_pdu_mux_header_length_get(&mux_header);
		sdu_ptr += mux_header.payload_length;
		remaining_len = pdu_end_ptr - sdu_ptr;
	}

	return true;
}

/**************************************************************************************************/

int dect_phy_mac_pdu_nw_beacon_period_in_ms(dect_phy_mac_nw_beacon_period_t period)
{
	switch (period) {
	case DECT_PHY_MAC_NW_BEACON_PERIOD_50MS:
		return 50;
	case DECT_PHY_MAC_NW_BEACON_PERIOD_100MS:
		return 100;
	case DECT_PHY_MAC_NW_BEACON_PERIOD_500MS:
		return 500;
	case DECT_PHY_MAC_NW_BEACON_PERIOD_1000MS:
		return 1000;
	case DECT_PHY_MAC_NW_BEACON_PERIOD_1500MS:
		return 1500;
	case DECT_PHY_MAC_NW_BEACON_PERIOD_2000MS:
		return 2000;
	case DECT_PHY_MAC_NW_BEACON_PERIOD_4000MS:
		return 4000;
	default:
		return -1;
	}
}

int dect_phy_mac_pdu_cluster_beacon_period_in_ms(dect_phy_mac_cluster_beacon_period_t period)
{
	switch (period) {
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_10MS:
		return 10;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_50MS:
		return 50;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_100MS:
		return 100;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_500MS:
		return 500;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_1000MS:
		return 1000;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_1500MS:
		return 1500;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_2000MS:
		return 2000;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_4000MS:
		return 4000;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_8000MS:
		return 8000;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_16000MS:
		return 16000;
	case DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_32000MS:
		return 32000;
	default:
		return -1;
	}
}

/**************************************************************************************************/

const char *
dect_phy_mac_pdu_cluster_beacon_repeat_string_get(dect_phy_mac_rach_repeat_type_t repeat)
{
	switch (repeat) {
	case DECT_PHY_MAC_RA_REPEAT_TYPE_SINGLE:
		return "Single allocation - no repetition and validity fields";
	case DECT_PHY_MAC_RA_REPEAT_TYPE_FRAMES:
		return "Repeated in the following frames as in repetition and validity fields";
	case DECT_PHY_MAC_RA_REPEAT_TYPE_SUBSLOTS:
		return "Repeated in the following subslots as in repetition and validity fields";
	case DECT_PHY_MAC_RA_REPEAT_TYPE_RESERVED:
		return "Reserved.";
	default:
		return "Unknown";
	}
}

/**************************************************************************************************/

const char *dect_phy_mac_pdu_association_req_setup_cause_string_get(
	dect_phy_mac_association_setup_cause_ie_t setup_cause)
{
	switch (setup_cause) {
	case DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_INIT:
		return "Initial association.";
	case DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_NEW_SET_OF_FLOWS:
		return "Association to request new set of flows.";
	case DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_MOBILITY:
		return "Association due to mobility.";
	case DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_AFTER_ERROR:
		return "Re-association after error.";
	case DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_CHANGE_OP_CHANNEL:
		return "Change of own operating channel.";
	case DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_CHANGE_OP_MODE:
		return "Change operating mode.";
	case DECT_PHY_MAC_ASSOCIATIONP_CAUSE_PAGING_RESP:
		return "Paging response.";

	default:
		return "Unknown";
	}
}

const char *dect_phy_mac_dlc_pdu_ie_type_string_get(int type)
{
	switch (type) {
	case DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITH_ROUTING:
		return "Data: DLC Service type 0 with a routing header";
	case DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING:
		return "Data: DLC Service type 0 without a routing header";
	case DECT_PHY_MAC_DLC_IE_TYPE_SERV_1_WITH_ROUTING:
		return "Data: DLC Service type 1 or 2 or 3 with a routing header";
	case DECT_PHY_MAC_DLC_IE_TYPE_SERV_1_WITHOUT_ROUTING:
		return "Data: DLC Service type 1 or 2 or 3 without a routing header";
	case DECT_PHY_MAC_DLC_IE_TYPE_TIMERS_CONF:
		return "DLC Timers configuration control IE";
	case DECT_PHY_MAC_DLC_IE_TYPE_ESCAPE:
		return "DLC escape";
	default:
		return "Unknown DLC IE type";
	}
}

const char *
dect_phy_mac_pdu_association_rel_cause_string_get(dect_phy_mac_association_rel_cause_t rel_cause)
{
	switch (rel_cause) {
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_CONN_TERMINATION:
		return "Connection termination.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_MOBILITY:
		return "Mobility.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_LONG_INACTIVITY:
		return "Long Inactivity.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_INCOMPATIBLE_CONFIG:
		return "Incompatible Configuration.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NO_HW_RESOURCES:
		return "No sufficient HW/memory resource.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NO_RADIO_RESOURCES:
		return "No sufficient radio resources.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_BAD_RADIO_QUALITY:
		return "Bad radio quality.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_SECURITY_ERROR:
		return "Security error.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_PT_SIDE_SHORT_RD_ID_CONFLICT:
		return "Short RD ID Conflict detected in PT side.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_FT_SIDE_SHORT_RD_ID_CONFLICT:
		return "Short RD ID Conflict detected in FT side.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NOT_ASOCIATED_PT:
		return "Not associated PT.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NOT_ASOCIATED_FT:
		return "Not associated FT.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NOT_IN_FT_MODE:
		return "Not operating in FT mode.";
	case DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_OTHER_ERROR:
		return "Other error.";

	default:
		return "Unknown";
	}
}

int dect_phy_mac_pdu_sdu_list_add_padding(
	uint8_t **pdu_ptr, sys_dlist_t *sdu_list, uint8_t padding_len)
{
	if (sdu_list == NULL) {
		return -EINVAL;
	}

	if (padding_len == 0) {
		/* Nothing to do here */
		return 0;
	}

	dect_phy_mac_sdu_t *padding_sdu_list_item =
		(dect_phy_mac_sdu_t *)k_calloc(1, sizeof(dect_phy_mac_sdu_t));
	if (padding_sdu_list_item == NULL) {
		return -ENOMEM;
	}

	dect_phy_mac_mux_header_t mux_header = {
		.mac_ext = DECT_PHY_MAC_EXT_8BIT_LEN,
		.ie_type = DECT_PHY_MAC_IE_TYPE_PADDING,
		.payload_length = padding_len - 2,
	};

	if (padding_len > 2) {
		mux_header.payload_length = padding_len - 2;

	} else {
		mux_header.mac_ext = DECT_PHY_MAC_EXT_SHORT_IE;
		mux_header.payload_length = padding_len - 1;
	}
	padding_sdu_list_item->mux_header = mux_header;
	padding_sdu_list_item->message_type = DECT_PHY_MAC_MESSAGE_PADDING;
	padding_sdu_list_item->message.common_msg.data_length = mux_header.payload_length;
	memset(padding_sdu_list_item->message.common_msg.data, 0, DECT_DATA_MAX_LEN);

	sys_dlist_append(sdu_list, &padding_sdu_list_item->dnode);
	*pdu_ptr = dect_phy_mac_pdu_sdus_encode(*pdu_ptr, sdu_list);

	return 0;
}
