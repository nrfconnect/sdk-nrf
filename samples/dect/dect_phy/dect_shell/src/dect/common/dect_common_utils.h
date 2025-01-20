/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_COMMON_UTILS_H
#define DECT_COMMON_UTILS_H

#define DECT_COMMON_UTILS_BIT_MASK_1BIT	 (0x01)
#define DECT_COMMON_UTILS_BIT_MASK_2BIT	 (0x03)
#define DECT_COMMON_UTILS_BIT_MASK_3BIT	 (0x07)
#define DECT_COMMON_UTILS_BIT_MASK_4BIT	 (0x0F)
#define DECT_COMMON_UTILS_BIT_MASK_5BIT	 (0x1F)
#define DECT_COMMON_UTILS_BIT_MASK_6BIT	 (0x3F)
#define DECT_COMMON_UTILS_BIT_MASK_7BIT	 (0x7F)
#define DECT_COMMON_UTILS_BIT_MASK_8BIT	 (0xFF)
#define DECT_COMMON_UTILS_BIT_MASK_12BIT (0x0FFF)
#define DECT_COMMON_UTILS_BIT_MASK_24BIT (0xFFFFFF)

/******************************************************************************/

bool dect_common_utils_channel_is_supported(uint16_t band_nbr, uint16_t channel,
					    bool use_harmonize_std);
uint16_t dect_common_utils_channel_max_on_band(uint16_t band_nbr);
uint16_t dect_common_utils_channel_min_on_band(uint16_t band_nbr);
uint16_t dect_common_utils_get_next_channel_in_band_range(uint16_t band_nbr,
							  uint16_t current_channel,
							  bool use_harmonize_std);

void dect_common_utils_fill_with_repeating_pattern(void *out, size_t outsize);

/******************************************************************************/
struct mapping_tbl_item {
	int key;
	char *value_str;
};

const char *dect_common_utils_map_to_string(struct mapping_tbl_item const *mapping_table, int mode,
					    char *out_str_buff);

const char *dect_common_utils_modem_phy_err_to_string(enum nrf_modem_dect_phy_err err,
						      int16_t temperature, char *out_str_buff);

const char *dect_common_utils_packet_length_type_to_string(int length_type, char *out_str_buff);

/******************************************************************************/

/* For networking protocol endianness */

uint16_t dect_common_utils_16bit_be_read(const uint8_t **pp_data);
uint32_t dect_common_utils_24bit_be_read(const uint8_t **data_ptr);
uint32_t dect_common_utils_32bit_be_read(const uint8_t **pp_data);
uint8_t *dect_common_utils_16bit_be_write(uint8_t *p_target, const uint16_t value);
uint8_t *dect_common_utils_24bit_be_write(uint8_t *p_target, const uint32_t value);
uint8_t *dect_common_utils_32bit_be_write(uint8_t *p_target, const uint32_t value);

/******************************************************************************/
/**
 * @brief Return the PHY header packet length for given bytes.
 *
 * Hardcoded for the case of mu=1, beta=1.
 *
 * See Table C.1-1 of [1]
 *
 * @param[in]  p_phy_header  Physical Layer Control field.
 *
 * @return  the packet length in bytes, or -1 if the header is invalid.
 */
int dect_common_utils_phy_packet_length_calculate(uint16_t bytes_to_send, uint8_t len_type,
						  uint8_t df_mcs);
int dect_common_utils_slots_in_bytes(uint8_t slots, uint8_t mcs);
int dect_common_utils_subslots_in_bytes(uint8_t subslots, uint8_t mcs);

int8_t dect_common_utils_phy_tx_power_to_dbm(uint8_t phy_power);
uint8_t dect_common_utils_dbm_to_phy_tx_power(int8_t pwr_dBm);
uint8_t dect_common_utils_next_phy_tx_power_get(uint8_t phy_power);

/******************************************************************************/

const char *
dect_common_utils_modem_phy_header_status_to_string(enum nrf_modem_dect_phy_hdr_status status,
						    char *out_str_buff);
const char *
dect_common_utils_modem_phy_header_status_to_string_short(enum nrf_modem_dect_phy_hdr_status status,
							  char *out_str_buff);
int8_t dect_common_utils_harq_tx_next_redundancy_version_get(uint8_t current_redundancy_version);

/******************************************************************************/

bool dect_common_utils_32bit_network_id_validate(uint32_t network_id);

/******************************************************************************/

bool dect_common_utils_mdm_ticks_is_in_range(uint64_t time, uint64_t start, uint64_t end);

/******************************************************************************/

const char *dect_common_utils_radio_mode_to_string(int mode, char *out_str_buff);

/******************************************************************************/

int8_t dect_common_utils_max_tx_pwr_dbm_by_pwr_class(uint8_t power_class);

#endif /* DECT_COMMON_UTILS_H */
