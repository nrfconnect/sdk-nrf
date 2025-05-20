/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_phy_api_scheduler.h"

uint16_t dect_common_utils_16bit_be_read(const uint8_t **data_ptr)
{
	const uint8_t *p_ptr = *data_ptr;
	*data_ptr += 2;

	return (p_ptr[0] << 8) + p_ptr[1];
}

uint32_t dect_common_utils_24bit_be_read(const uint8_t **data_ptr)
{
	const uint8_t *p_ptr = *data_ptr;

	*data_ptr += 3;
	return ((uint32_t)p_ptr[0] << 16) + ((uint32_t)p_ptr[1] << 8) + (uint32_t)p_ptr[2];
}

uint32_t dect_common_utils_32bit_be_read(const uint8_t **data_ptr)
{
	const uint8_t *p_ptr = *data_ptr;

	*data_ptr += 4;
	return ((uint32_t)p_ptr[0] << 24) + ((uint32_t)p_ptr[1] << 16) + ((uint32_t)p_ptr[2] << 8) +
	       (uint32_t)p_ptr[3];
}

uint8_t *dect_common_utils_16bit_be_write(uint8_t *p_target, const uint16_t value)
{
	*p_target++ = (value >> 8) & DECT_COMMON_UTILS_BIT_MASK_8BIT;
	*p_target++ = value & DECT_COMMON_UTILS_BIT_MASK_8BIT;

	return p_target;
}

uint8_t *dect_common_utils_24bit_be_write(uint8_t *p_target, const uint32_t value)
{
	*p_target++ = (value >> 16) & DECT_COMMON_UTILS_BIT_MASK_8BIT;
	*p_target++ = (value >> 8) & DECT_COMMON_UTILS_BIT_MASK_8BIT;
	*p_target++ = value & DECT_COMMON_UTILS_BIT_MASK_8BIT;

	return p_target;
}

uint8_t *dect_common_utils_32bit_be_write(uint8_t *p_target, const uint32_t value)
{
	*p_target++ = (value >> 24) & DECT_COMMON_UTILS_BIT_MASK_8BIT;
	*p_target++ = (value >> 16) & DECT_COMMON_UTILS_BIT_MASK_8BIT;
	*p_target++ = (value >> 8) & DECT_COMMON_UTILS_BIT_MASK_8BIT;
	*p_target++ = value & DECT_COMMON_UTILS_BIT_MASK_8BIT;

	return p_target;
}

/**************************************************************************************************/

/* Transport block sizes in bits for different packet lengths given in subslots.
 * Calculated based on section 5.3 of [1].
 */

#define N_SUPPORTED_SUBSLOT_PACKET_LENGTHS 16
#define N_SUPPORTED_SLOT_PACKET_LENGTHS	   8
#define N_SUPPORTED_MCS			   5
#define UNSUPPORTED_TBS			   -1

static int16_t const tbs_for_subslots[N_SUPPORTED_MCS][N_SUPPORTED_SUBSLOT_PACKET_LENGTHS] = {
	/* MCS-0: */
	{ 0, 136, 264, 400, 536, 664, 792, 920, 1064, 1192, 1320, 1448, 1576, 1704, 1864, 1992, },
	/* MCS-1: */
	{ 32, 296, 552, 824, 1096, 1352, 1608, 1864, 2104, 2360, 2616, 2872, 3128, 3384, 3704, 3960,
	},
	/* MCS-2: */
	{ 56, 456, 856, 1256, 1640, 2024, 2360, 2744, 3192, 3576, 3960, 4320, 4768, 5152, 5536, -1,
	},
	/* MCS-3: */
	{ 88, 616, 1128, 1672, 2168, 2680, 3192, 3704, 4256, 4768, 5280, -1, -1, -1, -1, -1, },
	/* MCS-4: */
	{ 144, 936, 1736, 2488, 3256, 4024, 4832, 5600, -1, -1, -1, -1, -1, -1, -1, -1, },
};

static int dect_common_utils_packet_length_bytes(uint16_t phy_pkt_len, uint8_t len_type,
						 uint8_t df_mcs)
{
	uint32_t subslot_index = phy_pkt_len;

	if (len_type == DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS) {
		if (subslot_index >= N_SUPPORTED_SLOT_PACKET_LENGTHS) {
			return -1;
		}
		/* mu == 1 -> packet_length means * 2 subslots. Use full slot indexes from
		 * tbs_for_subslots
		 */
		subslot_index = subslot_index * 2 + 1;
	}

	if (df_mcs >= N_SUPPORTED_MCS || subslot_index >= N_SUPPORTED_SUBSLOT_PACKET_LENGTHS) {
		return -1;
	}
	int packet_length_bits = tbs_for_subslots[df_mcs][subslot_index];

	if (packet_length_bits == UNSUPPORTED_TBS) {
		return -1;
	}

	return packet_length_bits / 8;
}

int dect_common_utils_slots_in_bytes(uint8_t slots, uint8_t mcs)
{
	int byte_count = 0;

	byte_count = dect_common_utils_packet_length_bytes(
		slots, DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS, mcs);
	return byte_count;
}

int dect_common_utils_subslots_in_bytes(uint8_t subslots, uint8_t mcs)
{
	int byte_count = 0;

	byte_count = dect_common_utils_packet_length_bytes(
		subslots, DECT_PHY_HEADER_PKT_LENGTH_TYPE_SUBSLOTS, mcs);
	return byte_count;
}

int dect_common_utils_phy_packet_length_calculate(uint16_t bytes_to_send, uint8_t len_type,
						  uint8_t df_mcs)
{
	uint8_t estimate = 0;
	uint8_t calculated = estimate;
	uint16_t tbs_in_bytes = 0;
	int i;

	for (i = estimate; tbs_in_bytes < bytes_to_send; i++) {
		calculated = i;
		tbs_in_bytes = dect_common_utils_packet_length_bytes(i, len_type, df_mcs);
		if (tbs_in_bytes < 0) {
			printk("Phy pkt length need calculation failed\n");
			return -1;
		}
	}
	return calculated;
}

int dect_common_utils_slots_to_tbs(uint8_t slots, uint8_t mcs)
{
	return dect_common_utils_slots_in_bytes(slots, mcs);
}

/**************************************************************************************************/

/* MAC spec: Table 6.2.1-3a */
static const int8_t m_dbm_power_tbl[] = {-40, -30, -20, -16, -12, -8, -4, 0,
					 4,   7,   10,	13,  16,  19, 21, 23};

int8_t dect_common_utils_phy_tx_power_to_dbm(uint8_t phy_power)
{
	const uint16_t table_size = ARRAY_SIZE(m_dbm_power_tbl);

	if (phy_power >= table_size) {
		printk("Phy TX pwr value out of range: %d\n", phy_power);
		phy_power = table_size - 1;
	}
	return m_dbm_power_tbl[phy_power];
}

uint8_t dect_common_utils_dbm_to_phy_tx_power(int8_t pwr_dBm)
{
	const uint16_t table_size = ARRAY_SIZE(m_dbm_power_tbl);

	for (uint16_t i = 0; i < table_size; i++) {
		if (m_dbm_power_tbl[i] >= pwr_dBm) {
			return i;
		}
	}
	return table_size - 1;
}

uint8_t dect_common_utils_next_phy_tx_power_get(uint8_t phy_power)
{
	const uint16_t table_size = ARRAY_SIZE(m_dbm_power_tbl);

	phy_power = phy_power + 1;
	if (phy_power >= table_size) {
		phy_power = table_size - 1;
	}
	return phy_power;
}

/**************************************************************************************************/

const char *dect_common_utils_map_to_string(struct mapping_tbl_item const *mapping_table, int mode,
					    char *out_str_buff)
{
	bool found = false;
	int i;

	for (i = 0; mapping_table[i].key != -1; i++) {
		if (mapping_table[i].key == mode) {
			found = true;
			break;
		}
	}

	if (!found) {
		sprintf(out_str_buff, "%d (unknown value, not converted to string)", mode);
	} else {
		strcpy(out_str_buff, mapping_table[i].value_str);
	}
	return out_str_buff;
}
const char *dect_common_utils_modem_phy_err_to_string(enum nrf_modem_dect_phy_err err,
						      int16_t temperature, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{NRF_MODEM_DECT_PHY_SUCCESS, "NRF_MODEM_DECT_PHY_SUCCESS"},
		{NRF_MODEM_DECT_PHY_ERR_LBT_CHANNEL_BUSY, "ERR_LBT_CHANNEL_BUSY"},
		{NRF_MODEM_DECT_PHY_ERR_UNSUPPORTED_OP, "ERR_UNSUPPORTED_OP"},
		{NRF_MODEM_DECT_PHY_ERR_NOT_FOUND, "ERR_NOT_FOUND"},
		{NRF_MODEM_DECT_PHY_ERR_NO_MEMORY, "ERR_NO_MEMORY"},
		{NRF_MODEM_DECT_PHY_ERR_NOT_ALLOWED, "ERR_NOT_ALLOWED"},
		{NRF_MODEM_DECT_PHY_OK_WITH_HARQ_RESET, "PHY_OK_WITH_HARQ_RESET"},
		{NRF_MODEM_DECT_PHY_ERR_OP_START_TIME_LATE, "ERR_OP_START_TIME_LATE"},
		{NRF_MODEM_DECT_PHY_ERR_LBT_START_TIME_LATE, "ERR_LBT_START_TIME_LATE"},
		{NRF_MODEM_DECT_PHY_ERR_RF_START_TIME_LATE, "ERR_RF_START_TIME_LATE"},
		{NRF_MODEM_DECT_PHY_ERR_INVALID_START_TIME, "ERR_INVALID_START_TIME"},
		{NRF_MODEM_DECT_PHY_ERR_OP_SCHEDULING_CONFLICT, "ERR_OP_SCHEDULING_CONFLICT"},
		{NRF_MODEM_DECT_PHY_ERR_OP_TIMEOUT, "ERR_OP_TIMEOUT"},
		{NRF_MODEM_DECT_PHY_ERR_NO_ONGOING_HARQ_RX, "ERR_NO_ONGOING_HARQ_RX"},
		{NRF_MODEM_DECT_PHY_ERR_PARAMETER_UNAVAILABLE, "ERR_PARAMETER_UNAVAILABLE"},
		{NRF_MODEM_DECT_PHY_ERR_PAYLOAD_UNAVAILABLE, "ERR_PAYLOAD_UNAVAILABLE"},
		{NRF_MODEM_DECT_PHY_ERR_OP_CANCELED, "PHY_ERR_OP_CANCELED"},
		{NRF_MODEM_DECT_PHY_ERR_COMBINED_OP_FAILED, "ERR_COMBINED_OP_FAILED"},
		{NRF_MODEM_DECT_PHY_ERR_RADIO_MODE_CONFLICT, "ERR_RADIO_MODE_CONFLICT"},
		{NRF_MODEM_DECT_PHY_ERR_UNSUPPORTED_CARRIER, "ERR_UNSUPPORTED_CARRIER"},
		{NRF_MODEM_DECT_PHY_ERR_UNSUPPORTED_DATA_SIZE, "ERR_UNSUPPORTED_DATA_SIZE"},
		{NRF_MODEM_DECT_PHY_ERR_INVALID_NETWORK_ID, "ERR_INVALID_NETWORK_ID"},
		{NRF_MODEM_DECT_PHY_ERR_INVALID_PHY_HEADER, "ERR_INVALID_PHY_HEADER"},
		{NRF_MODEM_DECT_PHY_ERR_INVALID_DURATION, "ERR_INVALID_DURATION"},
		{NRF_MODEM_DECT_PHY_ERR_INVALID_PARAMETER, "ERR_INVALID_PARAMETER"},
		{NRF_MODEM_DECT_PHY_ERR_TX_POWER_OVER_MAX_LIMIT, "ERR_TX_POWER_OVER_MAX_LIMIT"},
		{NRF_MODEM_DECT_PHY_ERR_MODEM_ERROR, "ERR_MODEM_ERROR"},
		{NRF_MODEM_DECT_PHY_ERR_MODEM_ERROR_RF_STATE, "ERR_MODEM_ERROR_RF_STATE"},
		{NRF_MODEM_DECT_PHY_ERR_TEMP_HIGH, "ERR_TEMP_HIGH"},
		{NRF_MODEM_DECT_PHY_ERR_PROD_LOCK, "ERR_PROD_LOCK"},
		/* Specific internal errors: */
		{DECT_SCHEDULER_DELAYED_ERROR, "DECT_SCHEDULER_DELAYED_ERROR"},
		{DECT_SCHEDULER_SCHEDULER_FATAL_MEM_ALLOC_ERROR,
		 "DECT_SCHEDULER_SCHEDULER_FATAL_MEM_ALLOC_ERROR"},
		{-1, NULL}};

	char temp_str[32];

	dect_common_utils_map_to_string(mapping_table, err, out_str_buff);
	if (temperature == NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED) {
		sprintf(temp_str, " (temperature: not measured)");
	} else {
		sprintf(temp_str, " (temperature %dC)", temperature);
	}
	strcat(out_str_buff, temp_str);

	return out_str_buff;
}

/**************************************************************************************************/

const char *dect_common_utils_packet_length_type_to_string(int length_type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{DECT_PHY_HEADER_PKT_LENGTH_TYPE_SUBSLOTS, "in subslots"},
		{DECT_PHY_HEADER_PKT_LENGTH_TYPE_SLOTS, "in slots"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, length_type, out_str_buff);
}

/**************************************************************************************************/

static bool dect_common_utils_is_odd_number(uint16_t number)
{
	if (number % 2) {
		return true; /* Odd number */
	} else {
		return false;
	}
}

static bool dect_common_utils_channel_is_in_band1_range(uint16_t channel)
{
	return (channel >= DECT_PHY_SUPPORTED_CHANNEL_BAND1_MIN &&
		channel <= DECT_PHY_SUPPORTED_CHANNEL_BAND1_MAX);
}

uint16_t dect_common_utils_get_next_channel_in_band_range(uint16_t band_nbr,
							  uint16_t current_channel,
							  bool use_harmonize_std)
{
	uint16_t next_channel;

	if (use_harmonize_std && band_nbr == 1) {
		/* Per harmonized std: only odd number channels at band #1:
		 * ETSI EN 301 406-2, V3.0.1, ch 4.3.2.3.
		 */
		if (dect_common_utils_is_odd_number(current_channel)) {
			next_channel = current_channel + 2;
		} else {
			next_channel = current_channel + 1;
		}
	} else {
		next_channel = current_channel + 1;
	}
	if (dect_common_utils_channel_is_supported(band_nbr, next_channel, use_harmonize_std)) {
		return next_channel;
	} else {
		return 0;
	}
}

bool dect_common_utils_channel_is_supported(uint16_t band_nbr, uint16_t channel,
					    bool use_harmonize_std)
{
	if (band_nbr == 1) {
		if (!dect_common_utils_channel_is_in_band1_range(channel)) {
			return false;
		}
		if (use_harmonize_std) {
			/* Only odd number channels: ETSI EN 301 406-2, V3.0.1, ch 4.3.2.3 */
			if (dect_common_utils_is_odd_number(channel)) {
				return true; /* Odd number */
			} else {
				return false;
			}
		} else {
			return true;
		}
	} else if (band_nbr == 2) {
		return (channel >= DECT_PHY_SUPPORTED_CHANNEL_BAND2_MIN &&
			channel <= DECT_PHY_SUPPORTED_CHANNEL_BAND2_MAX);
	} else if (band_nbr == 4) {
		return (channel >= DECT_PHY_SUPPORTED_CHANNEL_BAND4_MIN &&
			channel <= DECT_PHY_SUPPORTED_CHANNEL_BAND4_MAX);
	} else if (band_nbr == 9) {
		return (channel >= DECT_PHY_SUPPORTED_CHANNEL_BAND9_MIN &&
			channel <= DECT_PHY_SUPPORTED_CHANNEL_BAND9_MAX);
	} else if (band_nbr == 22) {
		return (channel >= DECT_PHY_SUPPORTED_CHANNEL_BAND22_MIN &&
			channel <= DECT_PHY_SUPPORTED_CHANNEL_BAND22_MAX);
	} else {
		return false;
	}
}

uint16_t dect_common_utils_channel_max_on_band(uint16_t band_nbr)
{
	if (band_nbr == 1) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND1_MAX;
	} else if (band_nbr == 2) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND2_MAX;
	} else if (band_nbr == 4) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND4_MAX;
	} else if (band_nbr == 9) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND9_MAX;
	} else if (band_nbr == 22) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND22_MAX;
	} else {
		return 0;
	}
}

uint16_t dect_common_utils_channel_min_on_band(uint16_t band_nbr)
{
	if (band_nbr == 1) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND1_MIN;
	} else if (band_nbr == 2) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND2_MIN;
	} else if (band_nbr == 4) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND4_MIN;
	} else if (band_nbr == 9) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND9_MIN;
	} else if (band_nbr == 22) {
		return DECT_PHY_SUPPORTED_CHANNEL_BAND22_MIN;
	} else {
		return 0;
	}
}

/**************************************************************************************************/

void dect_common_utils_fill_with_repeating_pattern(void *out, size_t outsize)
{
	size_t i;
	int counter = 0;
	char *buf = (char *)out;

	if (!outsize) {
		return;
	}

	for (i = 0; i < outsize; i++) {
		buf[i] = (char)('0' + counter);
		if (counter >= 9) {
			counter = 0;
		} else {
			counter++;
		}
	}
	buf[outsize - 1] = '\0'; /* null terminated string */
}

/**************************************************************************************************/

const char *
dect_common_utils_modem_phy_header_status_to_string_short(enum nrf_modem_dect_phy_hdr_status status,
							  char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{NRF_MODEM_DECT_PHY_HDR_STATUS_VALID, "valid"},
		{NRF_MODEM_DECT_PHY_HDR_STATUS_INVALID, "invalid"},
		{NRF_MODEM_DECT_PHY_HDR_STATUS_VALID_RX_END, "valid - but short"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, status, out_str_buff);
}

const char *
dect_common_utils_modem_phy_header_status_to_string(enum nrf_modem_dect_phy_hdr_status status,
						    char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{NRF_MODEM_DECT_PHY_HDR_STATUS_VALID, "valid - PDC can be received"},
		{NRF_MODEM_DECT_PHY_HDR_STATUS_INVALID, "invalid - PDC can't be received"},
		{NRF_MODEM_DECT_PHY_HDR_STATUS_VALID_RX_END,
		 "valid - but RX op ends before PDC reception"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, status, out_str_buff);
}

/**************************************************************************************************/

int8_t dect_common_utils_harq_tx_next_redundancy_version_get(uint8_t current_redundancy_version)
{
	/* MAC spec ch. 5.5.1:
	 * Hybrid ARQ redundancies shall be sent in the order {0, 2, 3, 1, 0, …}.
	 * Initial transmission shall use redundancy version 0 for the selected HARQ process number.
	 * Number of retransmissions is application dependent.
	 */
	if (current_redundancy_version == 0) {
		return 2;
	} else if (current_redundancy_version == 2) {
		return 3;
	} else if (current_redundancy_version == 3) {
		return 1;
	} else if (current_redundancy_version == 1) {
		return -1; /* We are done */
	} else {
		return -1; /* We are done */
	}
}

/**************************************************************************************************/

/* MAC spec ch, 4.2.3.1:
 * The network ID shall be set to a value where neither
 * the 8 LSB bits are 0x00 nor the 24 MSB bits are 0x000000.
 */

bool dect_common_utils_32bit_network_id_validate(uint32_t network_id)
{
	uint32_t msb_bits = network_id & 0xFFFFFF00; /* 24-bit MSB */
	uint8_t lsb_bits = network_id & 0xFF;	     /* 8-bit LSB */

	return ((0 != lsb_bits) && (0 != msb_bits));
}

/**************************************************************************************************/

bool dect_common_utils_mdm_ticks_is_in_range(uint64_t time, uint64_t start, uint64_t end)
{
	return (time >= start && time <= end);
}

/**************************************************************************************************/

const char *dect_common_utils_radio_mode_to_string(int mode, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY, "Low latency"},
		{NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY_WITH_STANDBY,
			"Low latency with standby"},
		{NRF_MODEM_DECT_PHY_RADIO_MODE_NON_LBT_WITH_STANDBY, "LBT disabled, with standby"},
		{-1, NULL}};

	return dect_common_utils_map_to_string(mapping_table, mode, out_str_buff);
}

/**************************************************************************************************/

int8_t dect_common_utils_max_tx_pwr_dbm_by_pwr_class(uint8_t power_class)
{
	switch (power_class) {
	case 1:
		return DECT_PHY_CLASS_1_MAX_TX_POWER_DBM;
	case 2:
		return DECT_PHY_CLASS_2_MAX_TX_POWER_DBM;
	case 3:
		return DECT_PHY_CLASS_3_MAX_TX_POWER_DBM;
	default:
		return DECT_PHY_CLASS_4_MAX_TX_POWER_DBM;
	}
}
