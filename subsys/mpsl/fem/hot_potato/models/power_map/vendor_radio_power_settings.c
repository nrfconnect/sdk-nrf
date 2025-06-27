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

#include <string.h>
#include <openthread/ncp.h>
#include <zephyr/storage/flash_map.h>

#include "vendor_radio_power_settings.h"
#include "vendor_radio_power_map.h"
#include "vendor_radio_power_limit.h"

#if USE_PARTITION_MANAGER
#include <pm_config.h>
#define FLASH_POWER_MAP_START_ADDRESS PM_POWER_MAP_TABLE_PARTITION_ADDRESS
#define FLASH_POWER_MAP_DATA_SIZE     PM_POWER_MAP_TABLE_PARTITION_SIZE
#define POWER_LIMIT_IDS_START_ADDRESS PM_POWER_LIMIT_IDS_PARTITION_ADDRESS
#define POWER_LIMIT_IDS_STOP_ADDRESS  PM_POWER_LIMIT_IDS_PARTITION_END_ADDRESS
#define POWER_LIMIT_IDS_SIZE	      PM_POWER_LIMIT_IDS_PARTITION_SIZE
#else
#define FLASH_POWER_MAP_START_ADDRESS DT_REG_ADDR(DT_NODELABEL(power_map_table_partition))
#define FLASH_POWER_MAP_DATA_SIZE     DT_REG_SIZE(DT_NODELABEL(power_map_table_partition))
#define POWER_LIMIT_IDS_START_ADDRESS DT_REG_ADDR(DT_NODELABEL(power_limit_ids_partition))
#define POWER_LIMIT_IDS_SIZE	      DT_REG_SIZE(DT_NODELABEL(power_limit_ids_partition))
#define POWER_LIMIT_IDS_STOP_ADDRESS  POWER_LIMIT_IDS_START_ADDRESS + POWER_LIMIT_IDS_SIZE
#endif

#define POWER_MAP_AREA	     FIXED_PARTITION_ID(power_map_table_partition)
#define POWER_LIMIT_IDS_AREA FIXED_PARTITION_ID(power_limit_ids_partition)

extern int16_t m_table_active_id;

static uint8_t *find_active_limit_id_addr(void)
{
#if FIXED_PARTITION_EXISTS(power_limit_ids_partition)
	uint8_t *addr = (uint8_t *)POWER_LIMIT_IDS_START_ADDRESS;

	while (addr < (const uint8_t *)POWER_LIMIT_IDS_STOP_ADDRESS && *addr != 0xFF) {
		addr += 1;
	}

	if (addr == (uint8_t *)POWER_LIMIT_IDS_START_ADDRESS) {
		// No active ids set - set to NULL as indicator
		addr = NULL;
	} else {
		addr -= 1;
	}

	return addr;
#else
	return NULL;
#endif
}

otError vendor_radio_power_map_limit_erase_flash(void)
{
	const struct flash_area *fa;
	otError result = OT_ERROR_NONE;

	if (flash_area_open(POWER_MAP_AREA, &fa) != 0) {
		return OT_ERROR_FAILED;
	}

	if (flash_area_erase(fa, 0, fa->fa_size) != 0) {
		result = OT_ERROR_FAILED;
	}

	flash_area_close(fa);
	return result;
}

otError vendor_radio_power_map_save_flash_data(void *p_data, uint16_t size, uint32_t address)
{
	uint8_t single_byte;
	const struct flash_area *fa;
	otError result = OT_ERROR_NONE;

	if (flash_area_open(POWER_MAP_AREA, &fa) != 0) {
		return OT_ERROR_FAILED;
	}

	// Check if memory is erased
	uint8_t erased_val = flash_area_erased_val(fa);
	for (uint16_t i = 0; i < size; i++) {
		if (flash_area_read(fa, address + i, &single_byte, 1) != 0) {
			flash_area_close(fa);
			return OT_ERROR_FAILED;
		}

		if (single_byte != erased_val) {
			flash_area_close(fa);
			return OT_ERROR_INVALID_STATE;
		}
	}

	if (flash_area_write(fa, address, p_data, size) != 0) {
		result = OT_ERROR_FAILED;
	}

	flash_area_close(fa);
	return result;
}

otError vendor_radio_power_map_read_flash_data(void *p_data, uint16_t size, uint32_t address)
{
	const struct flash_area *fa;

	if (flash_area_open(POWER_MAP_AREA, &fa) != 0) {
		return OT_ERROR_FAILED;
	}

	if (flash_area_read(fa, address, p_data, size) != 0) {
		flash_area_close(fa);
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError vendor_radio_power_limit_id_read(uint8_t *p_id)
{
	otError error = OT_ERROR_NOT_FOUND;
	uint8_t *id_ptr = find_active_limit_id_addr();

	if (id_ptr) {
		*p_id = *id_ptr;
		error = OT_ERROR_NONE;
	}

	return error;
}

void vendor_radio_power_limit_id_write(uint8_t id)
{
#if FIXED_PARTITION_EXISTS(power_limit_ids_partition)
	const struct flash_area *fa;
	uint32_t offset;
	uint8_t *current_ptr = find_active_limit_id_addr();

	if (flash_area_open(POWER_LIMIT_IDS_AREA, &fa) != 0) {
		return;
	}

	if (current_ptr == (const uint8_t *)POWER_LIMIT_IDS_STOP_ADDRESS - 1) {
		if (flash_area_erase(fa, 0, fa->fa_size) != 0) {
			flash_area_close(fa);
			return;
		}
		current_ptr = NULL;
	}

	current_ptr = current_ptr ? current_ptr + 1 : (uint8_t *)POWER_LIMIT_IDS_START_ADDRESS;
	offset = current_ptr - (uint8_t *)POWER_LIMIT_IDS_START_ADDRESS;

	if (flash_area_write(fa, offset, &id, sizeof(id)) != 0) {
		flash_area_close(fa);
		return;
	}

	flash_area_close(fa);
#endif
}

void vendor_radio_power_limit_id_erase(void)
{
#if FIXED_PARTITION_EXISTS(power_limit_ids_partition)
	const struct flash_area *fa;

	if (flash_area_open(POWER_LIMIT_IDS_AREA, &fa) != 0) {
		return;
	}

	if (flash_area_erase(fa, 0, fa->fa_size) != 0) {
		flash_area_close(fa);
		return;
	}

	flash_area_close(fa);
#endif
	m_table_active_id = DEFAULT_ACTIVE_ID;
}

uint8_t *vendor_radio_flash_memory_for_power_map_start_address_get(void)
{
	return (uint8_t *)FLASH_POWER_MAP_START_ADDRESS;
}

size_t vendor_radio_flash_memory_for_power_map_size_get(void)
{
	return FLASH_POWER_MAP_DATA_SIZE;
}

size_t vendor_radio_flash_memory_power_map_stored_size_get(void)
{
	uint8_t *map = vendor_radio_flash_memory_for_power_map_start_address_get();

	return map[0] * 256l + map[1];
}
