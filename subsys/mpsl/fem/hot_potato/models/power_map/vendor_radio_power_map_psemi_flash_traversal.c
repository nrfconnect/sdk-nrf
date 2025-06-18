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
 *   This file implements functions related to traversal of PSEMI FEM Power Table in flash.
 */

#include "vendor_radio_power_map_psemi_flash_traversal.h"

#include "vendor_const.h"
#include "vendor_radio_power_settings.h"
#include "utils.h"

#include <assert.h>
#include <string.h>

static uint8_t channel_to_id(uint8_t channel)
{
	return channel - MIN_802154_CHANNEL;
}

static size_t calculate_chip_tx_power_value_table_size(uint8_t chip_tx_power_count)
{
	return round_up_to_multiple_of(chip_tx_power_count, 2U) * sizeof(uint16_t);
}

static uint8_t *tx_path_entry_addr(uint8_t *p_power_map_start, tx_path_id_t id)
{
	uint8_t *addr = p_power_map_start;

	addr += sizeof(flash_power_map_header_t);

	switch (id) {
	case TX_PATH_ID_0:
		/* Nothing to do, TX_PATH_0 is just after the header. */
		break;

	case TX_PATH_ID_1: {
		uint8_t chip_tx_power_count =
			((flash_power_map_header_t *)p_power_map_start)->nChipTxPower0;
		size_t channel_entry_size = chip_tx_power_count * sizeof(flash_tx_power_entry_t);
		size_t channel_table_size =
			channel_entry_size * VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT;
		size_t chip_tx_power_value_table_size =
			calculate_chip_tx_power_value_table_size(chip_tx_power_count);
		size_t tx_path_entry_0_size = chip_tx_power_value_table_size + channel_table_size;

		/* TX_PATH_1 is after the header and TX_PATH_0. */
		addr += tx_path_entry_0_size;

		break;
	}

	default:
		assert(false);
	}

	return addr;
}

void power_map_get(power_map_ref_t *p_ref, void *p_power_map_start)
{
	assert(p_power_map_start != NULL);

	p_ref->p_flash_header = (flash_power_map_header_t *)p_power_map_start;
	p_ref->p_flash_tx_path_entry_0 = tx_path_entry_addr(p_power_map_start, 0);
	p_ref->p_flash_tx_path_entry_1 = tx_path_entry_addr(p_power_map_start, 1);
}

void tx_path_entry_get(tx_path_ref_t *p_ref, power_map_ref_t *p_power_map, tx_path_id_t id)
{
	switch (id) {
	case TX_PATH_ID_0:
		p_ref->p_flash = p_power_map->p_flash_tx_path_entry_0;
		p_ref->num_chip_tx_power = p_power_map->p_flash_header->nChipTxPower0;
		break;

	case TX_PATH_ID_1:
		p_ref->p_flash = p_power_map->p_flash_tx_path_entry_1;
		p_ref->num_chip_tx_power = p_power_map->p_flash_header->nChipTxPower1;
		break;

	default:
		assert(false);
	}
}

static inline bool is_channel_valid(uint8_t channel)
{
	return channel >= MIN_802154_CHANNEL && channel <= MAX_802154_CHANNEL;
}

static uint8_t *tx_path_get_channel_entry_addr(tx_path_ref_t *p_tx_path, uint8_t channel)
{
	size_t chip_tx_power_values_table_size =
		calculate_chip_tx_power_value_table_size(p_tx_path->num_chip_tx_power);
	size_t channel_entry_size = sizeof(flash_tx_power_entry_t) * p_tx_path->num_chip_tx_power;
	size_t channel_offset =
		chip_tx_power_values_table_size + channel_entry_size * channel_to_id(channel);

	return p_tx_path->p_flash + channel_offset;
}

void channel_entry_get(channel_entry_ref_t *p_ref, tx_path_ref_t *p_tx_path, uint8_t channel)
{
	assert(is_channel_valid(channel));

	p_ref->p_flash = tx_path_get_channel_entry_addr(p_tx_path, channel);
	p_ref->p_parent = p_tx_path;
}

void chip_tx_power_iter_init(chip_tx_power_iter_t *p_iter, channel_entry_ref_t *p_channel)
{
	p_iter->p_channel = p_channel;
	p_iter->next = (flash_tx_power_entry_t *)p_channel->p_flash;
	p_iter->end = p_iter->next + p_channel->p_parent->num_chip_tx_power;
}

uint16_t chip_tx_power_get(tx_path_ref_t *p_tx_path, uint16_t num_entry)
{
	return ntohs_signed(((uint16_t *)p_tx_path->p_flash)[num_entry]);
}

static uint8_t get_num_of_chip_tx_power_entry(chip_tx_power_iter_t *p_iter, uint8_t *entry)
{
	return (entry - p_iter->p_channel->p_flash) / sizeof(flash_tx_power_entry_t);
}

static inline uint8_t get_num_of_chip_tx_power_entries(chip_tx_power_iter_t *p_iter)
{
	return p_iter->p_channel->p_parent->num_chip_tx_power;
}

static void chip_tx_power_entry_ref_from_iter(chip_tx_power_entry_ref_t *p_ref,
					      chip_tx_power_iter_t *p_iter)
{
	uint8_t *curr = (uint8_t *)p_iter->next;
	size_t num_entry = get_num_of_chip_tx_power_entry(p_iter, curr);

	assert(num_entry < get_num_of_chip_tx_power_entries(p_iter));

	p_ref->p_flash = curr;
	p_ref->p_parent = p_iter->p_channel;
	p_ref->chip_tx_power = chip_tx_power_get(p_iter->p_channel->p_parent, num_entry);
}

static void chip_tx_power_iter_advance(chip_tx_power_iter_t *p_iter)
{
	p_iter->next++;

	if (p_iter->end <= p_iter->next) {
		p_iter->next = NULL;
	}
}

static bool chip_tx_power_iter_is_empty(chip_tx_power_iter_t *p_iter)
{
	return p_iter->next == NULL;
}

bool chip_tx_power_next(chip_tx_power_iter_t *p_iter, chip_tx_power_entry_ref_t *p_ref)
{
	if (chip_tx_power_iter_is_empty(p_iter)) {
		return false;
	}

	chip_tx_power_entry_ref_from_iter(p_ref, p_iter);

	chip_tx_power_iter_advance(p_iter);

	return true;
}

void att_iter_init(att_iter_t *p_iter, chip_tx_power_entry_ref_t *p_chip_power)
{
	p_iter->p_chip_power = p_chip_power;
	p_iter->next = (flash_att_entry_t *)p_chip_power->p_flash;
	p_iter->end = p_iter->next + NUM_ATT_STATES;
}

static uint8_t get_num_of_att_entry(att_iter_t *p_iter, uint8_t *entry)
{
	return (entry - p_iter->p_chip_power->p_flash) / sizeof(flash_att_entry_t);
}

static uint8_t att_state_from_att_entry_num(uint8_t entry_num)
{
	return MAX_ATT_STATE - entry_num;
}

static void att_entry_ref_from_iter(att_entry_ref_t *p_ref, att_iter_t *p_iter)
{
	uint8_t *curr = (uint8_t *)p_iter->next;
	size_t entry_num = get_num_of_att_entry(p_iter, curr);

	assert(entry_num < NUM_ATT_STATES);

	p_ref->p_flash = curr;
	p_ref->p_parent = p_iter->p_chip_power;
	p_ref->att_state = att_state_from_att_entry_num(entry_num);
}

static void att_iter_advance(att_iter_t *p_iter)
{
	p_iter->next++;

	if (p_iter->end <= p_iter->next) {
		p_iter->next = NULL;
	}
}

static bool att_iter_is_empty(att_iter_t *p_iter)
{
	return p_iter->next == NULL;
}

bool att_next(att_iter_t *p_iter, att_entry_ref_t *p_ref)
{
	if (att_iter_is_empty(p_iter)) {
		return false;
	}

	att_entry_ref_from_iter(p_ref, p_iter);

	att_iter_advance(p_iter);

	return true;
}
