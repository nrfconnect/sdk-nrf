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
 *   This file includes power map table related definitions common for various implementations.
 */

#ifndef VENDOR_RADIO_POWER_MAP_COMMON_H_
#define VENDOR_RADIO_POWER_MAP_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "openthread/error.h"
#include <stdint.h>
#include "vendor_radio_power_settings.h"
// #include "vendor_radio_default_configuration.h"

/**
 * @def VENDOR_RADIO_SETTINGS_POWER_AMPLIFIER_MAPPING_ARRAY_MAX_SIZE
 *
 * The maximum number of power amplifier settings.
 *
 */
#ifndef VENDOR_RADIO_SETTINGS_POWER_AMPLIFIER_MAPPING_ARRAY_MAX_SIZE
#define VENDOR_RADIO_SETTINGS_POWER_AMPLIFIER_MAPPING_ARRAY_MAX_SIZE 20
#endif

/**
 * @def VENDOR_RADIO_SETTINGS_FLASH_DATA_ALIGN_VALUE
 *
 * Radio power settings table header flash alignment value.
 *
 */
#ifndef VENDOR_RADIO_SETTINGS_FLASH_DATA_ALIGN_VALUE
#define VENDOR_RADIO_SETTINGS_FLASH_DATA_ALIGN_VALUE 4
#endif

typedef int8_t power_map_table_entry_t
	[VENDOR_RADIO_SETTINGS_POWER_AMPLIFIER_MAPPING_ARRAY_MAX_SIZE]; /**< Single power map array
									   entry.*/

typedef enum {
	POWER_MAP_STATUS_OK,			/** OK **/
	POWER_MAP_STATUS_MISSING,		/** No power map **/
	POWER_MAP_STATUS_PARSING_ERR,		/** Error while parsing **/
	POWER_MAP_STATUS_INVALID_LEN,		/** Invalid length **/
	POWER_MAP_STATUS_INVALID_CRC,		/** Invalid CRC **/
	POWER_MAP_STATUS_INVALID_VERSION,	/** Version not applicable for current FEM **/
	POWER_MAP_STATUS_MISSING_TX_POWER_PATH, /** TX power path 0 or TX power path 1 is missing */
} power_map_status_t;

typedef enum {
	POWER_MAP_VESRION_SKYWORKS = 1, /* Power map version used with Skyworks FEM */
	POWER_MAP_VESRION_PSEMI = 2,	/* Power map version used with PSEMI FEM */
} power_map_version_t;

/**
 * @brief Forward declaration of supported power maps.
 *
 */
struct power_map_psemi_data_struct_t;

typedef struct power_map_psemi_data_struct_t power_map_psemi_data_t;

/**
 * This method converts power map status to NULL terminated string.
 *
 * @param[in] status    A status to be converted.
 */
const char *vendor_radio_power_map_status_to_cstr(power_map_status_t status);

/**
 * This method returns the pointer where psemi data is stored.
 *
 */
power_map_psemi_data_t *vendor_radio_power_map_psemi_data_get(void);

#ifdef __cplusplus
}
#endif

#endif // VENDOR_RADIO_POWER_MAP_COMMON_H_
