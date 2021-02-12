/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
	uint32_t s0_address;
	uint32_t s1_address;
	uint32_t num_public_keys; /* Number of entries in 'key_data' list. */
	struct {
		uint32_t valid;
		uint8_t hash[CONFIG_SB_PUBLIC_KEY_HASH_LEN];
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
	uint16_t description; /* Counter description. What the counter is used for. See COUNTER_DESC_*. */
	uint16_t num_counter_slots; /* Number of entries in 'counter_slots' list. */
	uint16_t counter_slots[1];
};

/** The second data structure in the provision page. It has unknown length since
 *  'counters' is repeated. Note that each entry in counters also has unknown
 *  length, and each entry can have different length from the others, so the
 *  entries beyond the first cannot be accessed via array indices.
 */
struct counter_collection {
	uint16_t type; /* Must be "monotonic counter". */
	uint16_t num_counters; /* Number of entries in 'counters' list. */
	struct monotonic_counter counters[1];
};

#define TYPE_COUNTERS 1 /* Type referring to counter collection. */
#define COUNTER_DESC_VERSION 1 /* Counter description value for firmware version. */

static const struct bl_storage_data *p_bl_storage_data =
	(struct bl_storage_data *)PM_PROVISION_ADDRESS;

uint32_t s0_address_read(void)
{
	uint32_t addr = p_bl_storage_data->s0_address;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return addr;
}

uint32_t s1_address_read(void)
{
	uint32_t addr = p_bl_storage_data->s1_address;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return addr;
}

uint32_t num_public_keys_read(void)
{
	uint32_t num_pk = p_bl_storage_data->num_public_keys;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return num_pk;
}

/* Value written to the invalidation token when invalidating an entry. */
#define INVALID_VAL 0xFFFF0000

static bool key_is_valid(uint32_t key_idx)
{
	bool ret = (p_bl_storage_data->key_data[key_idx].valid != INVALID_VAL);

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return ret;
}

static uint16_t read_halfword(const uint16_t *ptr);

int verify_public_keys(void)
{
	for (uint32_t n = 0; n < num_public_keys_read(); n++) {
		if (key_is_valid(n)) {
			const uint16_t *p_key_n = (const uint16_t *)
					p_bl_storage_data->key_data[n].hash;
			size_t hash_len_u16 = (CONFIG_SB_PUBLIC_KEY_HASH_LEN/2);

			for (uint32_t i = 0; i < hash_len_u16; i++) {
				if (read_halfword(&p_key_n[i]) == 0xFFFF) {
					return -EHASHFF;
				}
			}
		}
	}
	return 0;
}

int public_key_data_read(uint32_t key_idx, uint8_t *p_buf, size_t buf_size)
{
	const uint8_t *p_key;

	if (!key_is_valid(key_idx)) {
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
	__ASSERT(((uint32_t)p_key % 4 == 0), "Key address is not word aligned");

	memcpy(p_buf, p_key, CONFIG_SB_PUBLIC_KEY_HASH_LEN);
	__DSB(); /* Because of nRF9160 Erratum 7 */

	return CONFIG_SB_PUBLIC_KEY_HASH_LEN;
}

void invalidate_public_key(uint32_t key_idx)
{
	const uint32_t *invalidation_token =
			&p_bl_storage_data->key_data[key_idx].valid;

	if (*invalidation_token != INVALID_VAL) {
		/* Write if not already written. */
		__DSB(); /* Because of nRF9160 Erratum 7 */
		nrfx_nvmc_word_write((uint32_t)invalidation_token, INVALID_VAL);
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
static uint16_t read_halfword(const uint16_t *ptr)
{
	bool top_half = ((uint32_t)ptr % 4); /* Addr not div by 4 */
	uint32_t target_addr = (uint32_t)ptr & ~3; /* Floor address */
	uint32_t val32 = *(uint32_t *)target_addr;
	__DSB(); /* Because of nRF9160 Erratum 7 */

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
static void write_halfword(const uint16_t *ptr, uint16_t val)
{
	bool top_half = (uint32_t)ptr % 4; /* Addr not div by 4 */
	uint32_t target_addr = (uint32_t)ptr & ~3; /* Floor address */

	uint32_t val32 = (uint32_t)val | 0xFFFF0000;
	uint32_t val32_shifted = ((uint32_t)val << 16) | 0x0000FFFF;

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
static const struct monotonic_counter *get_counter_struct(uint16_t description)
{
	const struct counter_collection *counters = get_counter_collection();

	if (counters == NULL) {
		return NULL;
	}

	const struct monotonic_counter *current = counters->counters;

	for (size_t i = 0; i < read_halfword(&counters->num_counters); i++) {
		uint16_t num_slots = read_halfword(&current->num_counter_slots);

		if (read_halfword(&current->description) == description) {
			return current;
		}

		current = (const struct monotonic_counter *)
					&current->counter_slots[num_slots];
	}
	return NULL;
}


uint16_t num_monotonic_counter_slots(void)
{
	const struct monotonic_counter *counter
			= get_counter_struct(COUNTER_DESC_VERSION);
	uint16_t num_slots = 0;

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
static uint16_t get_counter(const uint16_t **free_slot)
{
	uint16_t highest_counter = 0;
	const uint16_t *addr = NULL;
	const uint16_t *slots =
			get_counter_struct(COUNTER_DESC_VERSION)->counter_slots;
	uint16_t num_slots = num_monotonic_counter_slots();

	for (uint32_t i = 0; i < num_slots; i++) {
		uint16_t counter = ~read_halfword(&slots[i]);

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


uint16_t get_monotonic_counter(void)
{
	return get_counter(NULL);
}


int set_monotonic_counter(uint16_t new_counter)
{
	const uint16_t *next_counter_addr;
	uint16_t counter = get_counter(&next_counter_addr);

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
