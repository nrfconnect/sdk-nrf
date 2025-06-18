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
 *   This file includes all power map includes for easier management.
 */

#ifndef VENDOR_RADIO_POWER_MAP_H_
#define VENDOR_RADIO_POWER_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vendor_radio_power_settings.h"
#include "vendor_radio_power_map_common.h"

#include <stdint.h>

typedef enum
{
    POWER_MAP_TYPE_PSEMI = 1,
    POWER_MAP_TYPE_MAX,
} power_map_type_t;

typedef struct
{
    void (* vendor_radio_power_map_init)(void);
    otError (* vendor_radio_power_map_info_get)(uint16_t * p_version_information,
                                                uint8_t  * p_power_array_len);
    otError (* vendor_radio_power_map_internal_tx_power_get)(uint8_t   channel,
                                                             int8_t    requested_output_power,
                                                             int8_t  * p_calculated_power,
                                                             uint8_t * p_att_state,
                                                             int8_t  * p_final_power);
    otError (* vendor_radio_power_map_output_power_get)(uint8_t  channel,
                                                        int8_t   power_amplifier_power,
                                                        int8_t * p_output_power);
    otError (* vendor_radio_power_map_test)(uint8_t channel,
                                            int8_t requested_output_power,
                                            int8_t * p_power_amplifier_power,
                                            uint8_t * p_att_state,
                                            int8_t * p_output_power);
    power_map_status_t (* vendor_radio_power_map_flash_data_last_flash_validation_status)(void);
    power_map_status_t (* vendor_radio_power_map_flash_data_apply)(void);
    otError (* vendor_radio_power_map_clear_flash_data)(void);
    uint32_t (* vendor_radio_power_map_flash_data_max_size_get)(void);
    int8_t (* vendor_radio_power_map_att_to_gain)(uint8_t aAtt);
    power_map_status_t (* vendor_radio_power_map_flash_data_is_valid)(void);
    otError (* vendor_radio_power_map_tx_path_id_set)(uint8_t aTxPathId);
    uint8_t (* vendor_radio_power_map_tx_path_id_get)(void);
} power_map_api_t;

/**
 * @brief Initialize vendor radio power map module.
 *
 */
void vendor_radio_power_map_init(void);

/**
 * This method reads the power mapping table information.
 *
 * @param[in]  p_version_information    A pointer to where the power mapping table version information data is placed.
 * @param[in]  p_power_array_len        A pointer to where the number of power amplifier power table is placed.
 *
 * @retval OT_ERROR_NONE              Successfully read the header data.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 * @retval OT_ERROR_FAILED            Reading operation failed.
 * @retval OT_ERROR_NOT_FOUND         The flash memory is erased. The header data is invalid.
 *
 */
otError vendor_radio_power_map_info_get(uint16_t * p_version_information,
                                        uint8_t  * p_power_array_len);

/**
 * This method gets internal amplifier power value for specified output power based on the power map table.
 *
 * @param[in]  channel                Channel for which the power map table is selected.
 *                                    Channels have values from 11 to 26.
 * @param[in]  requested_output_power The requested output power for which the value will be calculated.
 * @param[out] p_calculated_power     A pointer to where the choosen internal amplifier power value is placed.
 * @param[out] p_att_state            A pointer to where the choosen external amplifier atetnuation/gain value is placed.
 *                                    Always returns 0 for Skyworks.
 * @param[out] p_final_power          A pointer to where the final transmission power value is placed.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 * @retval OT_ERROR_NOT_FOUND         Operation failed. The power mapping table does not exist or not selected.
 *
 */
otError vendor_radio_power_map_internal_tx_power_get(uint8_t   channel,
                                                     int8_t    requested_output_power,
                                                     int8_t  * p_calculated_power,
                                                     uint8_t * p_att_state,
                                                     int8_t  * p_final_power);

/**
 * This method gets output power for specified power amplifier power value based on the power map table.
 *
 * @param[in]  channel                Channel for which the power mapping table is selected.
 *                                    Channels have values from 11 to 26.
 * @param[in]  power_amplifier_power  The power amplifier power for which the output power value will be calculated.
 * @param[out] p_output_power         A pointer to where the choosen output power value is placed.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 * @retval OT_ERROR_NOT_FOUND         Operation failed. The power mapping table does not exist or not selected.
 *
 */
otError vendor_radio_power_map_output_power_get(uint8_t  channel,
                                                int8_t   power_amplifier_power,
                                                int8_t * p_output_power);

/**
 * This method test internal amplifier power value for specified output power based on the power map table.
 *
 * @param[in]  channel                 Channel for which the power map table is selected.
 *                                     Channels have values from 11 to 26.
 * @param[in]  requested_output_power  The requested output power for which the value will be calculated.
 * @param[out] p_power_amplifier_power A pointer to where the choosen internal amplifier power value is placed.
 *                                     If the pointer is NULL the requested_output_power value will be saved to memory.
 *                                     If the pointer is valid the requested_output_power from memory will be used for calculated.
 * @param[out] p_att_state             A pointer to where the choosen external power amplifier ATT pin state is placed.
 *                                     If there is no external power amplifier configuration UINT8_MAX will be returned.
 *                                     If the pointer is NULL the value will not be returned.
 * @param[out] p_output_power          A pointer to where the final transmission power value is placed.
 *
 * @retval OT_ERROR_NONE               Successfully gets the power.
 * @retval OT_ERROR_INVALID_ARGS       Invalid argument provided.
 * @retval OT_ERROR_NOT_FOUND          Operation failed. The power map table does not exist or not selected.
 *
 */
otError vendor_radio_power_map_test(uint8_t   channel,
                                    int8_t    requested_output_power,
                                    int8_t  * p_power_amplifier_power,
                                    uint8_t * p_att_state,
                                    int8_t  * p_output_power);

/**
 * This method checks if power mapping flash data is valid.
 *
 *
 * @retval POWER_MAP_STATUS_OK             Valid flash memory found in flash memory was applied as active.
 * @retval POWER_MAP_STATUS_MISSING        Power map was not found.
 * @retval POWER_MAP_STATUS_PARSING_ERR    Error occured while parsing power map.
 * @retval POWER_MAP_STATUS_INVALID_LEN    Length field is invalid.
 * @retval POWER_MAP_STATUS_INVALID_CRC    Calculated CRC does not match the provided one.*
 *
 */
power_map_status_t vendor_radio_power_map_flash_data_is_valid(void);
/**
 * This method return the status of the last flash power map validation status.
 *
 *
 * @retval POWER_MAP_STATUS_OK             Valid flash memory found in flash memory was applied as active.
 * @retval POWER_MAP_STATUS_MISSING        Power map was not found.
 * @retval POWER_MAP_STATUS_PARSING_ERR    Error occured while parsing power map.
 * @retval POWER_MAP_STATUS_INVALID_LEN    Length field is invalid.
 * @retval POWER_MAP_STATUS_INVALID_CRC    Calculated CRC does not match the provided one.
 *
 */
power_map_status_t vendor_radio_power_map_flash_data_last_flash_validation_status(void);

/**
 * This method read the flash data power map file and if valid apply as active power map configuration.
 *
 *
 * @retval POWER_MAP_STATUS_OK             Valid flash memory found in flash memory was applied as active.
 * @retval POWER_MAP_STATUS_MISSING        Power map was not found.
 * @retval POWER_MAP_STATUS_PARSING_ERR    Error occured while parsing power map.
 * @retval POWER_MAP_STATUS_INVALID_LEN    Length field is invalid.
 * @retval POWER_MAP_STATUS_INVALID_CRC    Calculated CRC does not match the provided one.
 *
 */
power_map_status_t vendor_radio_power_map_flash_data_apply(void);

/**
 * @brief Clear all power mapping table entries from flash memory.
 *
 * @retval OT_ERROR_NONE               Successfully cleared the flash memory.
 * @retval OT_ERROR_FAILED             Operation failed.
 *
 */
otError vendor_radio_power_map_clear_flash_data(void);

/**
 * This method converts power map status to NULL terminated string.
 *
 * @param[in] status    A status to be converted.
 */
const char * vendor_radio_power_map_status_to_cstr(power_map_status_t status);

/**
 * This method return maximal memory size of the power map in the flash.
 *
 * @retval  Power map flash memory size.
 */
uint32_t vendor_radio_power_map_flash_data_max_size_get(void);

/**
 * Convert ATT value to FEM gain.
 *
 * For SKYWORKS fixed gain value is returned.
 *
 * @param[in]  aAtt  ATT value from power map.
 *
 * @retval  Power gain for the given ATT configuration.
 */
int8_t vendor_radio_power_map_att_to_gain(uint8_t aAtt);

/**
 * Set tx path id.
 *
 * For SKYWORKS not needed.
 *
 * @param[in]  aTxPathId  A tx path index value for reading from power map.
 *
 * @retval OT_ERROR_NONE              Successfully sets the aTxPathId.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 * @retval OT_ERROR_NOT_IMPLEMENTED   Operation is not implemented.
 */
otError vendor_radio_power_map_tx_path_id_set(uint8_t aTxPathId);

/**
 * Get tx path id.
 *
 * For SKYWORKS not needed.
 *
 * @retval Currently selected tx path.

 */
uint8_t vendor_radio_power_map_tx_path_id_get(void);

#ifdef __cplusplus
}
#endif

#endif // VENDOR_RADIO_POWER_MAP_H_
