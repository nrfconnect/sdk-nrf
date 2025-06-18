/* Copyright (c) 2019, Nordic Semiconductor ASA
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
 *   This file implements power limit table related functions.
 */

#include <assert.h>
#include <string.h>

#include "vendor_radio_power_limit.h"
#include "vendor_radio_power_map.h"
#include "vendor_radio_default_configuration.h"

#define INVALID_POWER_LIMIT_TABLE_SELECTION -1

#define DEFAULT_ACTIVE_ID      0U
#define DEFAULT_TX_POWER_LIMIT 7U

static const power_limit_table_t m_table_5 = POWER_LIMIT_TABLE_5;
static const power_limit_table_t m_table_nrf_dk = POWER_LIMIT_TABLE_NRF_DK;
static int8_t m_default_tx_power_limit = DEFAULT_TX_POWER_LIMIT;

/**< Pointer to currently selected power limit table.*/
const power_limit_table_t *m_selected_table = NULL;

/**< Position of currently selected power limit table entry.*/
int16_t m_table_active_id = DEFAULT_ACTIVE_ID;

void vendor_radio_power_limit_init(int16_t selected_table_id)
{
	uint8_t active_id;

	if (selected_table_id != INVALID_POWER_LIMIT_TABLE_SELECTION) {
		m_selected_table = selected_table_id == PLT_ID_5 ? &m_table_5 : &m_table_nrf_dk;
	}

	if (vendor_radio_power_limit_id_read(&active_id) == OT_ERROR_NONE) {
		m_table_active_id = active_id;
	}
}

otError vendor_radio_power_limit_info_get(char *p_version_information, uint8_t *p_power_table_count)
{
	if (m_selected_table == NULL) {
		return OT_ERROR_FAILED;
	}

	memcpy(p_version_information, &m_selected_table->header.power_table_version,
	       strlen(m_selected_table->header.power_table_version) + 1);
	*p_power_table_count = m_selected_table->header.power_table_count;

	return OT_ERROR_NONE;
}

otError vendor_radio_power_limit_table_get(power_limit_table_data_t *p_table, uint16_t table_id,
					   uint8_t tx_path)
{
	if (m_selected_table == NULL) {
		return OT_ERROR_FAILED;
	}

	if ((p_table == NULL) || (table_id >= m_selected_table->header.power_table_count)) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (m_selected_table->header.power_table_paths > 0) {
		if (tx_path >= m_selected_table->header.power_table_paths) {
			return OT_ERROR_NOT_FOUND;
		}
		table_id += m_selected_table->header.power_table_count * tx_path;
	}

	memcpy(p_table,
	       (void *)((uint8_t *)m_selected_table->data +
			table_id * sizeof(power_limit_table_data_t)),
	       sizeof(power_limit_table_data_t));

	return OT_ERROR_NONE;
}

otError vendor_radio_power_limit_id_set(uint8_t id)
{
	otError result = OT_ERROR_NONE;

	if (m_selected_table == NULL) {
		result = OT_ERROR_FAILED;
	} else if (id >= m_selected_table->header.power_table_count) {
		result = OT_ERROR_INVALID_ARGS;
	} else if (m_table_active_id == id) {
		result = OT_ERROR_NONE;
	} else {
		vendor_radio_power_limit_id_write(id);
		m_table_active_id = id;
		// Restore TX power limitations in case they were removed.
		m_default_tx_power_limit = DEFAULT_TX_POWER_LIMIT;
	}

	return result;
}

otError vendor_radio_power_limit_id_invalidate(void)
{
	// Remove all TX power limitations.
	m_table_active_id = INVALID_POWER_LIMIT_TABLE_SELECTION;
	m_default_tx_power_limit = INT8_MAX;

	return OT_ERROR_NONE;
}

otError vendor_radio_power_limit_id_get(uint8_t *p_id)
{
	if (p_id == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (m_selected_table == NULL) {
		return OT_ERROR_FAILED;
	}

	if (m_table_active_id == INVALID_POWER_LIMIT_TABLE_SELECTION) {
		return OT_ERROR_NOT_FOUND;
	}

	*p_id = (uint8_t)m_table_active_id;

	return OT_ERROR_NONE;
}

otError vendor_radio_power_limit_get(uint8_t channel, uint8_t tx_path, int8_t *p_power)
{
	if ((p_power == NULL) || (channel < 11) || (channel > 26)) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (m_table_active_id == INVALID_POWER_LIMIT_TABLE_SELECTION || m_selected_table == NULL ||
	    m_table_active_id >= m_selected_table->header.power_table_count) {
		// If valid power map table is not selected, the returned power amplifier power
		// limit is set to m_default_tx_power_limit.
		*p_power = m_default_tx_power_limit;

		return OT_ERROR_NONE;
	}

	if (m_selected_table->header.power_table_paths > 0 &&
	    tx_path >= m_selected_table->header.power_table_paths) {
		return OT_ERROR_INVALID_ARGS;
	}

	*p_power = m_selected_table
			   ->data[m_table_active_id + m_selected_table->header.power_table_count *
							      tx_path][channel - 11];

	return OT_ERROR_NONE;
}

otError vendor_radio_power_limit_get_active_table_limits(power_limit_table_data_t *p_table)
{
	uint8_t id = 0;
	uint8_t path = 0;
	otError error = OT_ERROR_NONE;

	error = vendor_radio_power_limit_id_get(&id);
	path = vendor_radio_power_map_tx_path_id_get();

	if (error == OT_ERROR_NONE) {
		error = vendor_radio_power_limit_table_get(p_table, id, path);
	}

	return error;
}
