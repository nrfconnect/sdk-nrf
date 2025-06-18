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
 *   This file includes definitions related to traversal of PSEMI FEM Power Table in flash.
 */

#ifndef VENDOR_RADIO_POWER_MAP_PSEMI_FLASH_TRAVERSAL_H_
#define VENDOR_RADIO_POWER_MAP_PSEMI_FLASH_TRAVERSAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "vendor_radio_power_settings.h"

/**
 * @brief Structure representing a single ATT entry in flash.
 *
 */
typedef struct __attribute__((packed))
{
    uint8_t outPower;
} flash_att_entry_t;

/**
 * @brief Structure representing a single ATT entry in flash.
 *
 */
typedef struct __attribute__((packed))
{
    flash_att_entry_t entries[16];
} flash_tx_power_entry_t;

typedef struct __attribute__((packed))
{
    uint16_t lenght;
    uint16_t version;
    uint8_t  deviceType;
    uint8_t  calType;
    uint8_t  nChipTxPower0;
    uint8_t  nChipTxPower1;
} flash_power_map_header_t;

/**
 * @brief Structure representing a reference to Power Map in flash.
 *
 */
typedef struct
{
    flash_power_map_header_t * p_flash_header;
    uint8_t                  * p_flash_tx_path_entry_0;
    uint8_t                  * p_flash_tx_path_entry_1;
} power_map_ref_t;

/**
 * @brief Structure representing a reference to TX path in flash.
 *
 */
typedef struct
{
    uint8_t * p_flash;
    uint8_t   num_chip_tx_power;
} tx_path_ref_t;

/**
 * @brief Structure representing a reference to a single channel entry in flash.
 *
 */
typedef struct
{
    uint8_t       * p_flash;
    tx_path_ref_t * p_parent;
} channel_entry_ref_t;

/**
 * @brief Structure representing a reference to a single chip TX power entry in flash.
 *
 */
typedef struct
{
    uint8_t             * p_flash;
    channel_entry_ref_t * p_parent;
    int8_t                chip_tx_power;
} chip_tx_power_entry_ref_t;

/**
 * @brief Structure representing a reference to a single ATT entry in flash.
 *
 */
typedef struct
{
    uint8_t                   * p_flash;
    chip_tx_power_entry_ref_t * p_parent;
    uint8_t                     att_state;
} att_entry_ref_t;

/**
 * @brief Structure representing an iterator over chip TX power entries.
 *
 */
typedef struct
{
    channel_entry_ref_t    * p_channel;
    flash_tx_power_entry_t * next;
    flash_tx_power_entry_t * end;
} chip_tx_power_iter_t;

/**
 * @brief Structure representing an iterator over ATT entries.
 *
 */
typedef struct
{
    chip_tx_power_entry_ref_t * p_chip_power;
    flash_att_entry_t         * next;
    flash_att_entry_t         * end;
} att_iter_t;

/**
 * @brief Supported IDs of TX Paths.
 *
 */
typedef enum
{
    TX_PATH_ID_0 = 0,
    TX_PATH_ID_1,
    TX_PATH_COUNT,
} tx_path_id_t;

/**
 * @brief Gets Power Map reference.
 *
 * @param[out] p_ref             Pointer to Power Map reference struct.
 * @param[in]  p_power_map_start Address of Power Map memory.
 *
 */
void power_map_get(power_map_ref_t * p_ref, void * p_power_map_start);

/**
 * @brief Gets TX path entry reference.
 *
 * @param[out] p_ref       Pointer to TX path reference struct.
 * @param[in]  p_power_map Pointer to parent Power Map reference struct.
 * @param[in]  id          Id of requested TX path.
 *
 */
void tx_path_entry_get(tx_path_ref_t * p_ref, power_map_ref_t * p_power_map, tx_path_id_t id);

/**
 * @brief Gets channel entry reference.
 *
 * @param[out] p_ref     Pointer to channel reference struct.
 * @param[in]  p_tx_path Pointer to parent TX path reference struct.
 * @param[in]  channel   Channel number.
 *
 */
void channel_entry_get(channel_entry_ref_t * p_ref, tx_path_ref_t * p_tx_path, uint8_t channel);

/**
 * @brief Initializes an iterator over chip TX power values.
 *
 * @param[out] p_iter    Pointer to chip TX power iterator struct.
 * @param[in]  p_channel Pointer to parent channel reference struct.
 *
 */
void chip_tx_power_iter_init(chip_tx_power_iter_t * p_iter, channel_entry_ref_t * p_channel);

/**
 * @brief Returns next value of chip TX power reference struct.
 *
 * @param[in]  p_iter Pointer to chip TX power iterator struct.
 * @param[out] p_ref  Pointer to chip TX power reference struct to be populated.
 *
 * @retval true  Iterator was not empty, p_ref is populated.
 * @retval false Iterator is empty, p_ref was not changed.
 *
 */
bool chip_tx_power_next(chip_tx_power_iter_t * p_iter, chip_tx_power_entry_ref_t * p_ref);

/**
 * @brief Initializes an iterator over ATT entries for a given chip TX power.
 *
 * @param[out] p_iter       Pointer to ATT iterator struct.
 * @param[in]  p_chip_power Pointer to parent channel reference struct.
 *
 */
void att_iter_init(att_iter_t * p_iter, chip_tx_power_entry_ref_t * p_chip_power);

/**
 * @brief Returns next value of ATT reference struct.
 *
 * @param[in]  p_iter Pointer to ATT iterator struct.
 * @param[out] p_ref  Pointer to ATT reference struct to be populated.
 *
 * @retval true  Iterator was not empty, p_ref is populated.
 * @retval false Iterator is empty, p_ref was not changed.
 *
 */
bool att_next(att_iter_t * p_iter, att_entry_ref_t * p_ref);

#ifdef __cplusplus
}
#endif

#endif // VENDOR_RADIO_POWER_MAP_PSEMI_FLASH_TRAVERSAL_H_
