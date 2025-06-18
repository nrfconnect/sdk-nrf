/* Copyright (c) 2024, Nordic Semiconductor ASA
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
 *   This file implements power map table related functions.
 */
#include <assert.h>
#include <string.h>

#include "vendor_radio_power_map.h"
#include "openthread/platform/alarm-milli.h"

static power_map_api_t m_fem_api;

extern void vendor_radio_get_psemi_power_map_interface(power_map_api_t *p_api);

void vendor_radio_power_map_init(void)
{
	memset(&m_fem_api, 0, sizeof(m_fem_api));
	vendor_radio_get_psemi_power_map_interface(&m_fem_api);
	assert(m_fem_api.vendor_radio_power_map_init);
	m_fem_api.vendor_radio_power_map_init();
}

power_map_status_t vendor_radio_power_map_flash_data_is_valid(void)
{
	assert(m_fem_api.vendor_radio_power_map_flash_data_is_valid);

	return m_fem_api.vendor_radio_power_map_flash_data_is_valid();
}

power_map_status_t vendor_radio_power_map_flash_data_apply(void)
{
	assert(m_fem_api.vendor_radio_power_map_flash_data_apply);

	return m_fem_api.vendor_radio_power_map_flash_data_apply();
}

power_map_status_t vendor_radio_power_map_flash_data_last_flash_validation_status(void)
{
	assert(m_fem_api.vendor_radio_power_map_flash_data_last_flash_validation_status);

	return m_fem_api.vendor_radio_power_map_flash_data_last_flash_validation_status();
}

uint32_t vendor_radio_power_map_flash_data_max_size_get(void)
{
	assert(m_fem_api.vendor_radio_power_map_flash_data_max_size_get);

	return m_fem_api.vendor_radio_power_map_flash_data_max_size_get();
}

otError vendor_radio_power_map_clear_flash_data(void)
{
	assert(m_fem_api.vendor_radio_power_map_clear_flash_data);

	return m_fem_api.vendor_radio_power_map_clear_flash_data();
}

otError vendor_radio_power_map_info_get(uint16_t *p_version_information, uint8_t *p_power_array_len)
{
	assert(m_fem_api.vendor_radio_power_map_info_get);

	return m_fem_api.vendor_radio_power_map_info_get(p_version_information, p_power_array_len);
}

otError vendor_radio_power_map_test(uint8_t channel, int8_t requested_output_power,
				    int8_t *p_tx_power, uint8_t *p_att_state,
				    int8_t *p_output_power)
{
	assert(m_fem_api.vendor_radio_power_map_test);

	return m_fem_api.vendor_radio_power_map_test(channel, requested_output_power, p_tx_power,
						     p_att_state, p_output_power);
}

otError vendor_radio_power_map_internal_tx_power_get(uint8_t channel, int8_t requested_output_power,
						     int8_t *p_tx_power, uint8_t *p_att_state,
						     int8_t *p_final_power)
{
	assert(m_fem_api.vendor_radio_power_map_internal_tx_power_get);

	return m_fem_api.vendor_radio_power_map_internal_tx_power_get(
		channel, requested_output_power, p_tx_power, p_att_state, p_final_power);
}

int8_t vendor_radio_power_map_att_to_gain(uint8_t aAtt)
{
	assert(m_fem_api.vendor_radio_power_map_att_to_gain);

	return m_fem_api.vendor_radio_power_map_att_to_gain(aAtt);
}

otError vendor_radio_power_map_tx_path_id_set(uint8_t aTxPathId)
{
	assert(m_fem_api.vendor_radio_power_map_tx_path_id_set);

	return m_fem_api.vendor_radio_power_map_tx_path_id_set(aTxPathId);
}

uint8_t vendor_radio_power_map_tx_path_id_get(void)
{
	uint8_t path = 0;

	if (m_fem_api.vendor_radio_power_map_tx_path_id_get) {
		path = m_fem_api.vendor_radio_power_map_tx_path_id_get();
	}

	return path;
}