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
 *   This file includes power map table related definitions.
 */

#ifndef VENDOR_RADIO_POWER_MAP_PSEMI_H_
#define VENDOR_RADIO_POWER_MAP_PSEMI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "openthread/error.h"
#include <stdint.h>
#include "vendor_const.h"
#include "vendor_radio_power_settings.h"
#include "vendor_radio_power_map_common.h"
#include "vendor_radio_power_map_psemi_flash_traversal.h"

/**
 * @brief Structure representing a single out power resulting from the Power Map.
 *
 */
typedef struct
{
    int8_t  actual_power;      /**< Real power found in Power Map for this entry. */
    int8_t  radio_tx_power_db; /**< Value of RADIO peripheral's TX_POWER register used to achieve actual_power. */
    uint8_t att_state;         /**< FEM ATT pins' state used to achieve actual_power. */
} out_power_entry_t;

// psemi memory structure

typedef struct
{
    out_power_entry_t entries[NUM_SUPPORTED_POWERS];
} psemi_power_map_channel_entry_t;

typedef struct
{
    uint16_t                        version;
    psemi_power_map_channel_entry_t channels[VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT];
} psemi_power_map_table_t;

typedef struct
{
    const psemi_power_map_table_t * power_map_current; /**< Pointer to currently active power map table.*/
    power_map_status_t              power_map_status;  /**< Result of flash file validation.*/
} psemi_power_map_t;

struct power_map_psemi_data_struct_t
{
    void                  * mp_power_map_addr;
    power_map_ref_t         m_power_map_ref;
    psemi_power_map_table_t m_power_map_flash;
    psemi_power_map_t       m_power_map;
    tx_path_id_t            m_tx_path_id;
};

#ifdef __cplusplus
}
#endif

#endif // VENDOR_RADIO_POWER_MAP_PSEMI_H_
