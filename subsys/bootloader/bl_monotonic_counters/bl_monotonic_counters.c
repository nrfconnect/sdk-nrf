/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bl_monotonic_counters.h>
#include <string.h>
#include <errno.h>
#include <nrf.h>
#include <assert.h>
#include <nrfx_nvmc.h>

#ifdef CONFIG_SECURE_BOOT_STORAGE
#include <bl_storage.h>
#else
#include <pm_config.h>
#endif

#ifdef CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS
BUILD_ASSERT(CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS % 2 == 0,
			 "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS was not an even number");
#endif

#ifdef CONFIG_SB_NUM_VER_COUNTER_SLOTS
BUILD_ASSERT(CONFIG_SB_NUM_VER_COUNTER_SLOTS % 2 == 0,
			 "CONFIG_SB_NUM_VER_COUNTER_SLOTS was not an even number");
#endif


/** This library implements monotonic counters where each time the counter
 *  is increased, a new slot is written.
 *  This way, the counter can be updated without erase. This is, among other things,
 *  necessary so the counter can be used in the OTP section of the UICR
 *  (available on e.g. nRF91 and nRF53).
 */
struct monotonic_counter {
	uint16_t description; /* Counter description. What the counter is used for. See BL_MONOTONIC_COUNTERS_DESC_*. */
	uint16_t num_counter_slots; /* Number of entries in 'counter_slots' list. */
	uint16_t counter_slots[1];
};

/**  This struct is placed after the bl_storage_data struct if present. It has unknown length since
 *  'counters' is repeated. Note that each entry in counters also has unknown
 *  length, and each entry can have different length from the others, so the
 *  entries beyond the first cannot be accessed via array indices.
 */
struct counter_collection {
	uint16_t type; /* Must be "monotonic counter". */
	uint16_t num_counters; /* Number of entries in 'counters' list. */
	struct monotonic_counter counters[1];
};

/** Get the counter_collection data structure in the provision data. */
static const struct counter_collection *get_counter_collection(void)
{
#if defined(CONFIG_SECURE_BOOT_STORAGE)
	const struct counter_collection *collection = (struct counter_collection *)
		&BL_STORAGE->key_data[num_public_keys_read()];
#else
	const struct counter_collection *collection = (struct counter_collection *)PM_PROVISION_ADDRESS;
#endif

	return nrfx_nvmc_otp_halfword_read((uint32_t)&collection->type) == TYPE_COUNTERS
		? collection : NULL;
}


/** Get one of the (possibly multiple) counters in the provision data.
 *
 *  param[in]  description  Which counter to get. See COUNTER_DESC_*.
 */
static const struct monotonic_counter *get_counter_struct(uint16_t description)
{
	const struct counter_collection *cnt_collection = get_counter_collection();

	if (cnt_collection == NULL) {
		return NULL;
	}

	const struct monotonic_counter *current = cnt_collection->counters;

	for (size_t i = 0; i < nrfx_nvmc_otp_halfword_read((uint32_t)&cnt_collection->num_counters); i++) {

		if (nrfx_nvmc_otp_halfword_read((uint32_t)&current->description) == description) {
			return current;
		}

		uint16_t num_slots = nrfx_nvmc_otp_halfword_read((uint32_t)&current->num_counter_slots);
		current = (const struct monotonic_counter *)
					&current->counter_slots[num_slots];
	}
	return NULL;
}


int num_monotonic_counter_slots(uint16_t counter_desc, uint16_t *counter_slots)
{
	const struct monotonic_counter *counter
			= get_counter_struct(counter_desc);
	uint16_t num_slots = 0;

	if (counter == NULL || counter_slots == NULL) {
		return -EINVAL;
	}

	num_slots = nrfx_nvmc_otp_halfword_read((uint32_t)&counter->num_counter_slots);
	if (num_slots == 0xFFFF) {
		/* We consider the 0xFFFF as invalid since it is the default value of the OTP */
		*counter_slots = 0;
		return -EINVAL;
	}

	*counter_slots = num_slots;
	return 0;
}


/** Function for getting the current value and the first free slot.
 *
 * @param[in]   counter_desc  Counter description
 * @param[out]  counter_value The current value of the requested counter.
 * @param[out]  free_slot     Pointer to the first free slot. Can be NULL.
 *
 * @retval 0        Success
 * @retval -EINVAL  Cannot find counters with description @p counter_desc or the pointer to
 *                  @p counter_value is NULL.
 */
static int get_counter(uint16_t counter_desc, uint16_t *counter_value, const uint16_t **free_slot)
{
	uint16_t highest_counter = 0;
	const uint16_t *addr = NULL;
	const struct monotonic_counter *counter_obj = get_counter_struct(counter_desc);
	const uint16_t *slots;
	uint16_t num_counter_slots;

	if (counter_obj == NULL || counter_value == NULL){
		return -EINVAL;
	}

	slots = counter_obj->counter_slots;
	num_counter_slots = nrfx_nvmc_otp_halfword_read((uint32_t)&counter_obj->num_counter_slots);

	for (uint32_t i = 0; i < num_counter_slots; i++) {
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

	*counter_value = highest_counter;
	return 0;
}

int get_monotonic_counter(uint16_t counter_desc, uint16_t *counter_value)
{
	return get_counter(counter_desc, counter_value, NULL);
}

int set_monotonic_counter(uint16_t counter_desc, uint16_t new_counter)
{
	const uint16_t *next_counter_addr;
	uint16_t current_cnt_value;
	int err;

	err = get_counter(counter_desc, &current_cnt_value, &next_counter_addr);
	if (err != 0) {
		return err;
	}

	if (new_counter <= current_cnt_value) {
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
