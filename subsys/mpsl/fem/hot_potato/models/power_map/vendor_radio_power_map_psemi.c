/* Copyright (c) 2023, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements power map table related functions for PSEMI FEM.
 */

#include "vendor_radio_power_map_psemi.h"

#include "crc32.h"
#include "vendor_const.h"
#include "vendor_radio_default_configuration.h"
#include "vendor_radio_power_map.h"
#include "utils.h"

#include <assert.h>
#include <string.h>

#ifndef NRF_FEM_MAX_GAIN
#define NRF_FEM_MAX_GAIN 20
#endif

static const psemi_power_map_table_t m_power_map_default_fem_enabled =
	DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_ENABLED;
static const psemi_power_map_table_t m_power_map_default_fem_disabled =
	DEFAULT_POWER_MAP_TABLE_FEM_PSEMI_DISABLED;

static const psemi_power_map_table_t *mp_power_map_default_fem_disabled =
	&m_power_map_default_fem_disabled;
static const psemi_power_map_table_t *mp_power_map_default_fem_enabled =
	&m_power_map_default_fem_enabled;

static power_map_status_t vendor_radio_psemi_power_map_flash_data_is_valid(void);

static uint8_t channel_to_id(uint8_t channel)
{
	return channel - MIN_802154_CHANNEL;
}

static uint8_t power_to_id(uint8_t power)
{
	return power - MIN_SUPPORTED_POWER;
}

static uint16_t flash_power_map_get_length(void)
{
	return ntohs(
		vendor_radio_power_map_psemi_data_get()->m_power_map_ref.p_flash_header->lenght);
}

static uint16_t flash_power_map_get_version(void)
{
	return ntohs(
		vendor_radio_power_map_psemi_data_get()->m_power_map_ref.p_flash_header->version);
}

static uint8_t flash_power_map_get_nChipTxPower0(void)
{
	return vendor_radio_power_map_psemi_data_get()
		->m_power_map_ref.p_flash_header->nChipTxPower0;
}

static uint32_t flash_power_map_get_crc32(void)
{
	power_map_psemi_data_t *fem_context = vendor_radio_power_map_psemi_data_get();
	uint8_t *crc_addr = (uint8_t *)fem_context->m_power_map_ref.p_flash_header;

	crc_addr += flash_power_map_get_length() -
		    sizeof(fem_context->m_power_map_ref.p_flash_header->lenght);

	return (uint32_t)(crc_addr[0] | (crc_addr[1] << 8U) | (crc_addr[2] << 16U) |
			  (crc_addr[3] << 24U));
}

static bool is_length_empty(uint16_t length)
{
	return length == (uint16_t)0xffffU;
}

static int8_t out_power_find(channel_entry_ref_t *p_channel, int8_t requested_out_power,
			     int8_t *p_tx_power, uint8_t *p_att_state)
{
	chip_tx_power_iter_t power_iter;
	chip_tx_power_entry_ref_t power_entry;
	int8_t best_match_power = INT8_MIN;
	uint8_t att_state = MAX_ATT_STATE;
	int8_t tx_power = 0;

	chip_tx_power_iter_init(&power_iter, p_channel);

	while (chip_tx_power_next(&power_iter, &power_entry)) {
		att_iter_t att_iter;
		att_entry_ref_t att_entry;

		att_iter_init(&att_iter, &power_entry);

		while (att_next(&att_iter, &att_entry)) {
			int8_t curr_power = *(int8_t *)att_entry.p_flash;

			if ((best_match_power <= curr_power) &&
			    (requested_out_power >= curr_power)) {
				best_match_power = curr_power;
				att_state = att_entry.att_state;
				tx_power = power_entry.chip_tx_power;
			}

			if (curr_power == requested_out_power) {
				goto exit;
			}
		}
	}

exit:
	*p_tx_power = tx_power;
	*p_att_state = att_state;

	return best_match_power;
}

static void power_map_build_for_channel(channel_entry_ref_t *p_channel, out_power_entry_t *p_out)
{
	for (size_t power = MIN_SUPPORTED_POWER; power <= MAX_SUPPORTED_POWER; power++) {
		out_power_entry_t out_power = {0};

		out_power.actual_power = out_power_find(
			p_channel, power, &out_power.radio_tx_power_db, &out_power.att_state);

		*p_out++ = out_power;
	}
}

static power_map_status_t vendor_radio_power_map_flash_data_read(void)
{
	tx_path_ref_t tx_path;
	power_map_psemi_data_t *fem_context = vendor_radio_power_map_psemi_data_get();

	fem_context->m_power_map_flash.version = flash_power_map_get_version();

	tx_path_entry_get(&tx_path, &fem_context->m_power_map_ref, fem_context->m_tx_path_id);

	for (uint8_t channel_num = MIN_802154_CHANNEL; channel_num <= MAX_802154_CHANNEL;
	     channel_num++) {
		channel_entry_ref_t channel;

		channel_entry_get(&channel, &tx_path, channel_num);

		power_map_build_for_channel(&channel, fem_context->m_power_map_flash
							      .channels[channel_to_id(channel_num)]
							      .entries);
	}

	return POWER_MAP_STATUS_OK;
}

static power_map_status_t vendor_radio_psemi_power_map_flash_data_apply(void)
{
	power_map_status_t result = POWER_MAP_STATUS_OK;
	power_map_psemi_data_t *fem_context = vendor_radio_power_map_psemi_data_get();

	if ((result = vendor_radio_psemi_power_map_flash_data_is_valid()) == POWER_MAP_STATUS_OK &&
	    (result = vendor_radio_power_map_flash_data_read()) == POWER_MAP_STATUS_OK) {
		fem_context->m_power_map.power_map_current = &fem_context->m_power_map_flash;
	} else {
		{
			fem_context->m_power_map.power_map_current =
				mp_power_map_default_fem_enabled;
		}
	}

	return result;
}

static void vendor_radio_psemi_power_map_init(void)
{
	power_map_psemi_data_t *fem_context = vendor_radio_power_map_psemi_data_get();

	fem_context->m_tx_path_id = TX_PATH_ID_0;

	fem_context->mp_power_map_addr =
		vendor_radio_flash_memory_for_power_map_start_address_get();
	power_map_get(&fem_context->m_power_map_ref, fem_context->mp_power_map_addr);
	fem_context->m_power_map.power_map_status = vendor_radio_psemi_power_map_flash_data_apply();
}

static otError vendor_radio_psemi_power_map_clear_flash_data(void)
{
	otError error = OT_ERROR_NONE;

	error = vendor_radio_power_map_limit_erase_flash();
	if (error != OT_ERROR_NONE) {
		goto exit;
	}
	vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current =
		mp_power_map_default_fem_disabled;

exit:
	return error;
}

static otError vendor_radio_psemi_power_map_info_get(uint16_t *p_version_information,
						     uint8_t *p_power_array_len)
{
	if (p_version_information == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	*p_version_information =
		vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_current->version;

	if (p_power_array_len) {
		*p_power_array_len = TX_PATH_COUNT;
	}

	return OT_ERROR_NONE;
}

static void vendor_radio_psemi_power_map_find_settings_for_power(uint8_t channel,
								 int8_t requested_output_power,
								 out_power_entry_t *p_out_power)
{
	if (requested_output_power < MIN_SUPPORTED_POWER) {
		requested_output_power = MIN_SUPPORTED_POWER;
	} else if (requested_output_power > MAX_SUPPORTED_POWER) {
		requested_output_power = MAX_SUPPORTED_POWER;
	}

	uint8_t channel_id = channel_to_id(channel);
	uint8_t power_id = power_to_id(requested_output_power);

	*p_out_power = vendor_radio_power_map_psemi_data_get()
			       ->m_power_map.power_map_current->channels[channel_id]
			       .entries[power_id];
}

static power_map_status_t vendor_radio_psemi_power_map_flash_data_last_flash_validation_status(void)
{
	return vendor_radio_power_map_psemi_data_get()->m_power_map.power_map_status;
}

static uint32_t vendor_radio_psemi_power_map_flash_data_max_size_get(void)
{
	return vendor_radio_flash_memory_for_power_map_size_get();
}

static otError vendor_radio_psemi_power_map_internal_tx_power_get(uint8_t channel,
								  int8_t requested_output_power,
								  int8_t *p_tx_power,
								  uint8_t *p_att_state,
								  int8_t *p_final_power)
{
	otError error = OT_ERROR_NONE;
	out_power_entry_t out_power_entry;

	if (channel >= MIN_802154_CHANNEL && channel <= MAX_802154_CHANNEL && p_tx_power) {
		vendor_radio_psemi_power_map_find_settings_for_power(
			channel, requested_output_power, &out_power_entry);
		*p_tx_power = out_power_entry.radio_tx_power_db;

		if (p_att_state) {
			*p_att_state = out_power_entry.att_state;
		}
		if (p_final_power) {
			*p_final_power = out_power_entry.actual_power;
		}

		if (*p_tx_power == INT8_MIN) {
			error = OT_ERROR_NOT_FOUND;
		}
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	return error;
}

static otError vendor_radio_psemi_power_map_test(uint8_t channel, int8_t requested_output_power,
						 int8_t *p_tx_power, uint8_t *p_att_state,
						 int8_t *p_output_power)
{
	static int8_t power_map_test_power = INT8_MIN;
	otError error = OT_ERROR_NONE;

	if (p_tx_power == NULL) {
		power_map_test_power = requested_output_power; // store power for later
	} else {
		error = vendor_radio_psemi_power_map_internal_tx_power_get(
			channel, power_map_test_power, p_tx_power, p_att_state, p_output_power);
	}

	return error;
}

static int8_t vendor_radio_psemi_power_map_att_to_gain(uint8_t aAtt)
{
	return NRF_FEM_MAX_GAIN - aAtt;
}

static power_map_status_t vendor_radio_psemi_power_map_flash_data_is_valid(void)
{
	uint32_t crc_calc = 0;
	uint32_t read_crc = 0;
	uint16_t length = flash_power_map_get_length();
	uint16_t version = flash_power_map_get_version();
	uint8_t n_chip_tx_power_0 = flash_power_map_get_nChipTxPower0();
	size_t length_field_size = sizeof(uint16_t);
	size_t crc_size = length - length_field_size;

	if (length == 0) {
		return POWER_MAP_STATUS_PARSING_ERR;
	}
	if (is_length_empty(length)) {
		return POWER_MAP_STATUS_MISSING;
	}
	if (version != POWER_MAP_VESRION_PSEMI) {
		return POWER_MAP_STATUS_INVALID_VERSION;
	}
	// FEM PSEMI in nRF Connect SDK requires only one TX path
	if (n_chip_tx_power_0 == 0) {
		return POWER_MAP_STATUS_MISSING_TX_POWER_PATH;
	}

	crc_calc = crc32_compute(vendor_radio_power_map_psemi_data_get()->mp_power_map_addr,
				 crc_size, NULL);
	read_crc = flash_power_map_get_crc32();

	return (read_crc == crc_calc) ? POWER_MAP_STATUS_OK : POWER_MAP_STATUS_INVALID_CRC;
}

static otError vendor_radio_psemi_power_map_tx_path_id_set(uint8_t tx_path_id)
{
	power_map_psemi_data_t *fem_context = vendor_radio_power_map_psemi_data_get();

	switch (tx_path_id) {
	case TX_PATH_ID_0:
	case TX_PATH_ID_1:
		fem_context->m_tx_path_id = tx_path_id;
		break;

	default:
		return OT_ERROR_INVALID_ARGS;
	}

	return OT_ERROR_NONE;
}

static uint8_t vendor_radio_psemi_power_map_tx_path_id_get(void)
{
	power_map_psemi_data_t *fem_context = vendor_radio_power_map_psemi_data_get();

	return fem_context->m_tx_path_id;
}

void vendor_radio_get_psemi_power_map_interface(power_map_api_t *p_api)
{
	memset(p_api, 0, sizeof(*p_api));

	p_api->vendor_radio_power_map_init = vendor_radio_psemi_power_map_init;
	p_api->vendor_radio_power_map_info_get = vendor_radio_psemi_power_map_info_get;
	p_api->vendor_radio_power_map_internal_tx_power_get =
		vendor_radio_psemi_power_map_internal_tx_power_get;
	p_api->vendor_radio_power_map_output_power_get = NULL;
	p_api->vendor_radio_power_map_test = vendor_radio_psemi_power_map_test;
	p_api->vendor_radio_power_map_flash_data_last_flash_validation_status =
		vendor_radio_psemi_power_map_flash_data_last_flash_validation_status;
	p_api->vendor_radio_power_map_flash_data_apply =
		vendor_radio_psemi_power_map_flash_data_apply;
	p_api->vendor_radio_power_map_clear_flash_data =
		vendor_radio_psemi_power_map_clear_flash_data;
	p_api->vendor_radio_power_map_flash_data_max_size_get =
		vendor_radio_psemi_power_map_flash_data_max_size_get;
	p_api->vendor_radio_power_map_att_to_gain = vendor_radio_psemi_power_map_att_to_gain;
	p_api->vendor_radio_power_map_flash_data_is_valid =
		vendor_radio_psemi_power_map_flash_data_is_valid;
	p_api->vendor_radio_power_map_tx_path_id_set = vendor_radio_psemi_power_map_tx_path_id_set;
	p_api->vendor_radio_power_map_tx_path_id_get = vendor_radio_psemi_power_map_tx_path_id_get;
}
