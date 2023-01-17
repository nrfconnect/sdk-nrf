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
#include <nrfx_nvmc.h>
#include <pm_config.h>

#define TYPE_COUNTERS 1 /* Type referring to counter collection. */
#define COUNTER_DESC_VERSION 1 /* Counter description value for firmware version. */

#ifdef CONFIG_SB_NUM_VER_COUNTER_SLOTS
BUILD_ASSERT(CONFIG_SB_NUM_VER_COUNTER_SLOTS % 2 == 0,
			 "CONFIG_SB_NUM_VER_COUNTER_SLOTS must be an even number");
#endif

/** This library implements monotonic counters where each time the counter
 *  is increased, a new slot is written.
 *  This way, the counter can be updated without erase. This is, among other things,
 *  necessary so the counter can be used in the OTP section of the UICR
 *  (available on e.g. nRF91 and nRF53).
 */
struct monotonic_counter {
	/* Counter description. What the counter is used for. See
	 * BL_MONOTONIC_COUNTERS_DESC_x.
	 */
	uint16_t description;
	/* Number of entries in 'counter_slots' list. */
	uint16_t num_counter_slots;
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

uint32_t s0_address_read(void)
{
	uint32_t addr = BL_STORAGE->s0_address;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return addr;
}

uint32_t s1_address_read(void)
{
	uint32_t addr = BL_STORAGE->s1_address;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return addr;
}

uint32_t num_public_keys_read(void)
{
	uint32_t num_pk = BL_STORAGE->num_public_keys;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return num_pk;
}

/* Value written to the invalidation token when invalidating an entry. */
#define INVALID_VAL 0xFFFF0000

static bool key_is_valid(uint32_t key_idx)
{
	bool ret = (BL_STORAGE->key_data[key_idx].valid != INVALID_VAL);

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return ret;
}

int verify_public_keys(void)
{
	for (uint32_t n = 0; n < num_public_keys_read(); n++) {
		if (key_is_valid(n)) {
			for (uint32_t i = 0; i < SB_PUBLIC_KEY_HASH_LEN / 2; i++) {
				const uint16_t *hash_as_halfwords =
					(const uint16_t *)BL_STORAGE->key_data[n].hash;
				uint16_t halfword = nrfx_nvmc_otp_halfword_read(
					(uint32_t)&hash_as_halfwords[i]);
				if (halfword == 0xFFFF) {
					return -EHASHFF;
				}
			}
		}
	}
	return 0;
}

int public_key_data_read(uint32_t key_idx, uint8_t *p_buf)
{
	const volatile uint8_t *p_key;

	if (!key_is_valid(key_idx)) {
		return -EINVAL;
	}

	if (key_idx >= num_public_keys_read()) {
		return -EFAULT;
	}

	p_key = BL_STORAGE->key_data[key_idx].hash;

	/* Ensure word alignment, since the data is stored in memory region
	 * with word sized read limitation. Perform both build time and run
	 * time asserts to catch the issue as soon as possible.
	 */
	BUILD_ASSERT(offsetof(struct bl_storage_data, key_data) % 4 == 0);
	__ASSERT(((uint32_t)p_key % 4 == 0), "Key address is not word aligned");

	otp_copy32(p_buf, (volatile uint32_t *restrict)p_key, SB_PUBLIC_KEY_HASH_LEN);

	return SB_PUBLIC_KEY_HASH_LEN;
}

void invalidate_public_key(uint32_t key_idx)
{
	const volatile uint32_t *invalidation_token =
			&BL_STORAGE->key_data[key_idx].valid;

	if (*invalidation_token != INVALID_VAL) {
		/* Write if not already written. */
		__DSB(); /* Because of nRF9160 Erratum 7 */
		nrfx_nvmc_word_write((uint32_t)invalidation_token, INVALID_VAL);
	}
}

/** Get the counter_collection data structure in the provision data. */
static const struct counter_collection *get_counter_collection(void)
{
	const struct counter_collection *collection = (struct counter_collection *)
		&BL_STORAGE->key_data[num_public_keys_read()];
	return nrfx_nvmc_otp_halfword_read((uint32_t)&collection->type) == TYPE_COUNTERS
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

	for (size_t i = 0; i < nrfx_nvmc_otp_halfword_read(
		(uint32_t)&counters->num_counters); i++) {
		uint16_t num_slots = nrfx_nvmc_otp_halfword_read(
					(uint32_t)&current->num_counter_slots);

		if (nrfx_nvmc_otp_halfword_read((uint32_t)&current->description) == description) {
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
		num_slots = nrfx_nvmc_otp_halfword_read((uint32_t)&counter->num_counter_slots);
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
		uint16_t counter = ~nrfx_nvmc_otp_halfword_read((uint32_t)&slots[i]);

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

	nrfx_nvmc_halfword_write((uint32_t)next_counter_addr, ~new_counter);
	return 0;
}
