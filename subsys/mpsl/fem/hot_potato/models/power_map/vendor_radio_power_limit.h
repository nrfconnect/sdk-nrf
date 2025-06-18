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
 *   This file includes power limit table related definitions.
 */

#ifndef _VENDOR_RADIO_POWER_LIMIT_H_
#define _VENDOR_RADIO_POWER_LIMIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "vendor_radio_power_settings.h"

#define DEFAULT_ACTIVE_ID 0U /**< Default value of Active ID. */

/**
 * @def VENDOR_RADIO_POWER_LIMIT_MAX_SIZE
 *
 * The maximum length of power table version information.
 *
 */
#ifndef VENDOR_RADIO_POWER_LIMIT_MAX_SIZE
#define VENDOR_RADIO_POWER_LIMIT_MAX_SIZE 30
#endif

enum {
	PLT_ID_5 = 5, /**< Power limit table 5. */
};

typedef struct {
	char power_table_version[VENDOR_RADIO_POWER_LIMIT_MAX_SIZE]; /**< Power limit table version
									information field. */
	uint8_t power_table_count; /**< Number of the power limit tables for a single path. */
	uint8_t power_table_paths; /**< Number of the power limit paths. */
} power_limit_table_header_t;

typedef int8_t
	power_limit_table_data_t[VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT]; /**< Single power limit
										array entry.*/

typedef struct {
	power_limit_table_header_t header; /**< Power limit table header. */
	power_limit_table_data_t data[];   /**< Power limit table. */
} power_limit_table_t;

/**
 * @brief Initialize vendor radio power limit module.
 *
 * @param[in]  selected_table_id   Identifier of the selected power limit table identifier.
 */
void vendor_radio_power_limit_init(int16_t selected_table_id);

/**
 * @brief Select the proper power limit table.
 *
 * @param[in]  id   Identifier of the selected power limit table.
 *
 * @retval OT_ERROR_NONE              Successfully selected new power limit table.
 * @retval OT_ERROR_INVALID_ARGS      Invalid identifier provided.
 *
 */
otError vendor_radio_power_limit_id_set(uint8_t id);

/**
 * @brief Invalidate selection of power limit table.
 *
 * @retval OT_ERROR_NONE              Successfully invalidated power limit table.
 *
 */
otError vendor_radio_power_limit_id_invalidate(void);

/**
 * @brief Get currently selected power limit table identifier.
 *
 * @param[out]  p_id   A pointer where the identifier of the selected power limit table is placed.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power limit table identifier.
 * @retval OT_ERROR_NOT_FOUND         The active power limit tablie identifier is not set.
 * @retval OT_ERROR_INVALID_ARGS      Invalid identifier provided.
 *
 */
otError vendor_radio_power_limit_id_get(uint8_t *p_id);

/**
 * This method reads the power limit table version information and number of power limit table
 * entries.
 *
 * @param[in]  p_version_information  A pointer to buffer where the power limit table version
 * information string is placed.
 * @param[in]  p_power_table_size     A pointer to buffer where the total number of power limit
 * table entries is placed.
 *
 * @retval OT_ERROR_NONE              Successfully read the header data.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 *
 */
otError vendor_radio_power_limit_info_get(char *p_version_information, uint8_t *p_power_table_size);

/**
 * This method reads the power limit table entry.
 *
 * @param[in]  p_table                A pointer to buffer where the table data is placed.
 * @param[in]  table_id               Table identifier to be read.
 * @param[in]  tx_path                Path identifier to be read. Ignored for map version "1".
 *
 * @retval OT_ERROR_NONE              Successfully read the power limit table data.
 * @retval OT_ERROR_NOT_FOUND         The active power limit tablie identifier is not set.
 *
 */
otError vendor_radio_power_limit_table_get(power_limit_table_data_t *p_table, uint16_t table_id,
					   uint8_t tx_path);

/**
 * This method gets radio power limit for specified radio channel and selected power table.
 *
 * @param[in]  channel                Channel for which the radio power value is get.
 *                                    Channels have values from 11 to 26.
 * @param[in]  tx_path                Tx path for which the radio power value is get.
 * @param[out] p_power                A pointer to buffer where the radio power value is placed.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 *
 */
otError vendor_radio_power_limit_get(uint8_t channel, uint8_t tx_path, int8_t *p_power);

/**
 * This method gets the active power limit table values.
 *
 * @param[in]  p_table                A pointer to buffer where the table data is placed.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power.
 * @retval OT_ERROR_FAILED            Failed to retreive the data.
 */
otError vendor_radio_power_limit_get_active_table_limits(power_limit_table_data_t *p_table);

#ifdef __cplusplus
}
#endif

#endif // _VENDOR_RADIO_POWER_LIMIT_H_
