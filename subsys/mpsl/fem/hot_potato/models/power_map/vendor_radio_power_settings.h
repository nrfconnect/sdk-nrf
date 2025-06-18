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
 *   This file includes power map and limit table settings related definitions.
 */

#ifndef _VENDOR_RADIO_POWER_SETTINGS_H_
#define _VENDOR_RADIO_POWER_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "openthread/error.h"

#include <stdint.h>
#include <stddef.h>

/**
 * @def VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT
 *
 * The maximum number of radio channels.
 * The channels are corresponding to 802.15.4 radio channels 11 to 26.
 *
 */
#ifndef VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT
#define VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT 16
#endif

/**
 * @brief Clear all power mapping table entries from flash memory.
 *
 * @retval OT_ERROR_NONE               Successfully cleared the flash memory.
 * @retval OT_ERROR_FAILED             Operation failed.
 *
 */
otError vendor_radio_power_map_limit_erase_flash(void);

/**
 * This method saves power mapping data to flash memory.
 *
 * @param[in]  p_data                 A pointer with buffer which data will be saved.
 * @param[in]  size                   Number of bytes to be saved.
 * @param[in]  address                Flash memory address.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power.
 * @retval OT_ERROR_INVALID_STATE     The flash memory is not erased.
 * @retval OT_ERROR_FAILED            Operation failed.
 *
 */
otError vendor_radio_power_map_save_flash_data(void *p_data, uint16_t size, uint32_t address);

/**
 * This method gets power mapping data from flash memory.
 *
 * @param[out] p_data                 A pointer to buffer where data will be placed.
 * @param[in]  size                   Number of bytes to be readed.
 * @param[in]  address                Flash memory address.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power.
 * @retval OT_ERROR_INVALID_ARGS      Invalid argument provided.
 * @retval OT_ERROR_FAILED            Operation failed. The flash memory is erased or is invalid.
 *
 */
otError vendor_radio_power_map_read_flash_data(void *p_data, uint16_t size, uint32_t address);

/**
 * This method gets power limit active table id from flash memory.
 *
 * @param[out] p_id                   Returned power limit active table id.
 *
 * @retval OT_ERROR_NONE              Successfully gets the power limit active id.
 * @retval OT_ERROR_NOT_FOUND         There is no active id setting stored in the flash memory.
 *
 */
otError vendor_radio_power_limit_id_read(uint8_t *p_id);

/**
 * This method save power limit active table id to flash memory.
 *
 * @param[in] id                      Power limit active table id value to be saved.
 *
 */
void vendor_radio_power_limit_id_write(uint8_t id);

/**
 * This function clears Power Limit IDs memory region.
 *
 */
void vendor_radio_power_limit_id_erase();

/**
 * Gets the start address of flash memory designated for Power Map store.
 *
 * return Start address of Power Map flash memory.
 *
 */
uint8_t *vendor_radio_flash_memory_for_power_map_start_address_get(void);

/**
 * Gets size of flash memory designated for Power Map store.
 *
 * @return Size in bytes.
 *
 */
size_t vendor_radio_flash_memory_for_power_map_size_get(void);

/**
 * Gets size of the Power Map currently stored in flash.
 *
 * @return Power map size in bytes excluding 'size' field.
 *
 */
size_t vendor_radio_flash_memory_power_map_stored_size_get(void);

#ifdef __cplusplus
}
#endif

#endif // _VENDOR_RADIO_POWER_SETTINGS_H_
