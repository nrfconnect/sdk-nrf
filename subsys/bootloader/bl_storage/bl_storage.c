/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "bl_storage.h"
#include <string.h>
#include <errno.h>
#include <nrf.h>
#include <assert.h>
#include <pm_config.h>
#include <nrfx_nvmc.h>


/** The first data structure in the bootloader storage. It has unknown length
 *  since 'key_data' is repeated. This data structure is immediately followed by
 *  struct counter_collection.
 */
struct bl_storage_data {
	u32_t s0_address;
	u32_t s1_address;
	u32_t num_public_keys; /* Number of entries in 'key_data' list. */
	struct {
		u32_t valid;
		u8_t hash[CONFIG_SB_PUBLIC_KEY_HASH_LEN];
	} key_data[1];
};

/** A single monotonic counter. It consists of a description value, a 'type',
 *  to know what this counter counts. Further, it has a number of slots
 *  allocated to it. Each time the counter is updated, a new slot is written.
 *  This way, the counter can be updated without erasing anything. This is
 *  necessary so the counter can be used in the OTP section of the UICR
 *  (available on e.g. nRF91 and nRF53).
 */
struct monotonic_counter {
	u16_t description; /* Counter description. What the counter is used for. See COUNTER_DESC_*. */
	u16_t num_counter_slots; /* Number of entries in 'counter_slots' list. */
	u16_t counter_slots[1];
};

/** The second data structure in the provision page. It has unknown length since
 *  'counters' is repeated. Note that each entry in counters also has unknown
 *  length, and each entry can have different length from the others, so the
 *  entries beyond the first cannot be accessed via array indices.
 */
struct counter_collection {
	u16_t type; /* Must be "monotonic counter". */
	u16_t num_counters; /* Number of entries in 'counters' list. */
	struct monotonic_counter counters[1];
};

#define TYPE_COUNTERS 1 /* Type referring to counter collection. */
#define COUNTER_DESC_VERSION 1 /* Counter description value for firmware version. */

static const struct bl_storage_data *p_bl_storage_data =
	(struct bl_storage_data *)PM_PROVISION_ADDRESS;

u32_t s0_address_read(void)
{
	return p_bl_storage_data->s0_address;
}

u32_t s1_address_read(void)
{
	return p_bl_storage_data->s1_address;
}

u32_t num_public_keys_read(void)
{
	return p_bl_storage_data->num_public_keys;
}


/* Value written to the invalidation token when invalidating an entry. */
#define INVALID_VAL 0xFFFF0000

int public_key_data_read(u32_t key_idx, u8_t *p_buf, size_t buf_size)
{
	const u8_t *p_key;

	if (p_bl_storage_data->key_data[key_idx].valid == INVALID_VAL) {
		return -EINVAL;
	}

	if (buf_size < CONFIG_SB_PUBLIC_KEY_HASH_LEN) {
		return -ENOMEM;
	}

	if (key_idx >= num_public_keys_read()) {
		return -EFAULT;
	}

	p_key = p_bl_storage_data->key_data[key_idx].hash;

	/* Ensure word alignment, since the data is stored in memory region
	 * with word sized read limitation. Perform both build time and run
	 * time asserts to catch the issue as soon as possible.
	 */
	BUILD_ASSERT(CONFIG_SB_PUBLIC_KEY_HASH_LEN % 4 == 0);
	BUILD_ASSERT(offsetof(struct bl_storage_data, key_data) % 4 == 0);
	__ASSERT(((u32_t)p_key % 4 == 0), "Key address is not word aligned");

	memcpy(p_buf, p_key, CONFIG_SB_PUBLIC_KEY_HASH_LEN);

	return CONFIG_SB_PUBLIC_KEY_HASH_LEN;
}

void invalidate_public_key(u32_t key_idx)
{
	const u32_t *invalidation_token =
			&p_bl_storage_data->key_data[key_idx].valid;

	if (*invalidation_token != INVALID_VAL) {
		/* Write if not already written. */
		nrfx_nvmc_word_write((u32_t)invalidation_token, INVALID_VAL);
	}
}


/** Function for reading a half-word (2 byte value) from OTP.
 *
 * @details The flash in OTP supports only aligned 4 byte read operations.
 *          This function reads the encompassing 4 byte word, and returns the
 *          requested half-word.
 *
 * @param[in]  ptr  Address to read.
 *
 * @return value
 */
static u16_t read_halfword(const u16_t *ptr)
{
	bool top_half = ((u32_t)ptr % 4); /* Addr not div by 4 */
	u32_t target_addr = (u32_t)ptr & ~3; /* Floor address */
	u32_t val32 = *(u32_t *)target_addr;

	return (top_half ? (val32 >> 16) : val32) & 0x0000FFFF;
}


/** Function for writing a half-word (2 byte value) to OTP.
 *
 * @details The flash in OTP supports only aligned 4 byte write operations.
 *          This function writes to the encompassing 4 byte word, masking the
 *          other half-word with 0xFFFF so it is left untouched.
 *
 * @param[in]  ptr  Address to write to.
 * @param[in]  val  Value to write into @p ptr.
 */
static void write_halfword(const u16_t *ptr, u16_t val)
{
	bool top_half = (u32_t)ptr % 4; /* Addr not div by 4 */
	u32_t target_addr = (u32_t)ptr & ~3; /* Floor address */

	u32_t val32 = (u32_t)val | 0xFFFF0000;
	u32_t val32_shifted = ((u32_t)val << 16) | 0x0000FFFF;

	nrfx_nvmc_word_write(target_addr, top_half ? val32_shifted : val32);
}


/** Get the counter_collection data structure in the provision data. */
static const struct counter_collection *get_counter_collection(void)
{
	struct counter_collection *collection = (struct counter_collection *)
		&p_bl_storage_data->key_data[num_public_keys_read()];
	return read_halfword(&collection->type) == TYPE_COUNTERS
		? collection : NULL;
}


/** Get one of the (possibly multiple) counters in the provision data.
 *
 *  param[in]  description  Which counter to get. See COUNTER_DESC_*.
 */
static const struct monotonic_counter *get_counter_struct(u16_t description)
{
	const struct counter_collection *counters = get_counter_collection();

	if (counters == NULL) {
		return NULL;
	}

	const struct monotonic_counter *current = counters->counters;

	for (size_t i = 0; i < read_halfword(&counters->num_counters); i++) {
		u16_t num_slots = read_halfword(&current->num_counter_slots);

		if (read_halfword(&current->description) == description) {
			return current;
		}

		current = (const struct monotonic_counter *)
					&current->counter_slots[num_slots];
	}
	return NULL;
}


u16_t num_monotonic_counter_slots(void)
{
	const struct monotonic_counter *counter
			= get_counter_struct(COUNTER_DESC_VERSION);
	u16_t num_slots = 0;

	if (counter != NULL) {
		num_slots = read_halfword(&counter->num_counter_slots);
	}
	return num_slots != 0xFFFF ? num_slots : 0;
}


/** Function for getting the current value and the first free slot.
 *
 * @param[out]  free_slot  Pointer to the first free slot. Can be NULL.
 *
 * @return The current value of the counter (the highest value before the first
 *         free slot).
 */
static u16_t get_counter(const u16_t **free_slot)
{
	u16_t highest_counter = 0;
	const u16_t *addr = NULL;
	const u16_t *slots =
			get_counter_struct(COUNTER_DESC_VERSION)->counter_slots;
	u16_t num_slots = num_monotonic_counter_slots();

	for (u32_t i = 0; i < num_slots; i++) {
		u16_t counter = ~read_halfword(&slots[i]);

		if (counter == 0) {
			addr = &slots[i];
			break;
		}
		if (highest_counter < counter) {
			highest_counter = counter;
		}
	}

	if (free_slot != NULL) {
		*free_slot = addr;
	}
	return highest_counter;
}


u16_t get_monotonic_counter(void)
{
	return get_counter(NULL);
}


int set_monotonic_counter(u16_t new_counter)
{
	const u16_t *next_counter_addr;
	u16_t counter = get_counter(&next_counter_addr);

	if (new_counter <= counter) {
		/* Counter value must increase. */
		return -EINVAL;
	}

	if (next_counter_addr == NULL) {
		/* No more room. */
		return -ENOMEM;
	}

	write_halfword(next_counter_addr, ~new_counter);
	return 0;
}
