/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bl_storage.h>
#include <zephyr/autoconf.h>
#include <string.h>
#include <errno.h>
#include <nrf.h>
#include <assert.h>
#if defined(CONFIG_BUILD_WITH_TFM)
#include <stdlib.h>
#else
#include <zephyr/kernel.h>
#endif

#define TYPE_COUNTERS 1 /* Type referring to counter collection. */
#define COUNTER_DESC_VERSION 1 /* Counter description value for firmware version. */

#define STATE_ENTERED 0x0000
#define STATE_NOT_ENTERED 0xFFFF

#ifdef CONFIG_SB_NUM_VER_COUNTER_SLOTS
BUILD_ASSERT(CONFIG_SB_NUM_VER_COUNTER_SLOTS % 2 == 0,
			 "CONFIG_SB_NUM_VER_COUNTER_SLOTS must be an even number");
#endif

#ifdef CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS
BUILD_ASSERT(CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS % 2 == 0,
			 "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS was not an even number");
#endif

#if defined(CONFIG_NRFX_RRAMC)
static uint32_t index_from_address(uint32_t address)
{
	return ((address - (uint32_t)BL_STORAGE)/sizeof(uint32_t));
}
#endif

static counter_t bl_storage_counter_get(uint32_t address)
{
#if defined(CONFIG_NRFX_NVMC)
	return ~nrfx_nvmc_otp_halfword_read(address);
#elif defined(CONFIG_NRFX_RRAMC)
	return ~nrfx_rramc_otp_word_read(index_from_address(address));
#endif
}

static void bl_storage_counter_set(uint32_t address, counter_t value)
{
#if defined(CONFIG_NRFX_NVMC)
	nrfx_nvmc_halfword_write((uint32_t)address, ~value);
#elif defined(CONFIG_NRFX_RRAMC)
	nrfx_rramc_otp_word_write(index_from_address((uint32_t)address), ~value);
#endif
}

static uint32_t bl_storage_word_read(uint32_t address)
{
#if defined(CONFIG_NRFX_NVMC)
	return nrfx_nvmc_uicr_word_read((uint32_t *)address);
#elif defined(CONFIG_NRFX_RRAMC)
	return nrfx_rramc_word_read(address);
#endif
}

static uint32_t bl_storage_word_write(uint32_t address, uint32_t value)
{
#if defined(CONFIG_NRFX_NVMC)
	nrfx_nvmc_word_write(address, value);
	return 0;
#elif defined(CONFIG_NRFX_RRAMC)
	nrfx_rramc_word_write(address, value);
	return 0;
#endif
}

static uint16_t bl_storage_otp_halfword_read(uint32_t address)
{
	uint16_t halfword;
#if defined(CONFIG_NRFX_NVMC)
	halfword = nrfx_nvmc_otp_halfword_read(address);
#elif defined(CONFIG_NRFX_RRAMC)
	uint32_t word = nrfx_rramc_otp_word_read(index_from_address(address));

	if (!(address & 0x3)) {
		halfword = (uint16_t)(word & 0x0000FFFF); /* C truncates the upper bits */
	} else {
		halfword = (uint16_t)(word >> 16); /* Shift the upper half down */
	}
#endif
	return halfword;
}

/*
 * BL_STORAGE is usually, but not always, in UICR. For code simplicity
 * we read it as if it were a UICR address as it is safe (although
 * inefficient) to do so.
 */
uint32_t s0_address_read(void)
{
	return bl_storage_word_read((uint32_t)&BL_STORAGE->s0_address);
}

uint32_t s1_address_read(void)
{
	return bl_storage_word_read((uint32_t)&BL_STORAGE->s1_address);
}

uint32_t num_public_keys_read(void)
{
	return bl_storage_word_read((uint32_t)&BL_STORAGE->num_public_keys);
}

/* Value written to the invalidation token when invalidating an entry. */
#define INVALID_VAL 0x50FA0000
#define INVALID_WRITE_VAL 0xFFFF0000
#define VALID_VAL 0x50FAFFFF
#define MAX_NUM_PUBLIC_KEYS 16
#define TOKEN_IDX_OFFSET 24

static bool key_is_valid(uint32_t key_idx)
{
	if (key_idx >= num_public_keys_read()) {
		return false;
	}

	const uint32_t token = bl_storage_word_read((uint32_t)&BL_STORAGE->key_data[key_idx].valid);
	const uint32_t invalid_val = INVALID_VAL | key_idx << TOKEN_IDX_OFFSET;
	const uint32_t valid_val = VALID_VAL | key_idx << TOKEN_IDX_OFFSET;

	if (token == invalid_val) {
		return false;
	} else if (token == valid_val) {
		return true;
	}

	/* Invalid value. */
#if defined(CONFIG_BUILD_WITH_TFM)
	abort();
#else
	k_panic();
#endif
	return false;
}

int verify_public_keys(void)
{
	if (num_public_keys_read() > MAX_NUM_PUBLIC_KEYS) {
		return -EINVAL;
	}

	for (uint32_t n = 0; n < num_public_keys_read(); n++) {
		if (key_is_valid(n)) {
			for (uint32_t i = 0; i < SB_PUBLIC_KEY_HASH_LEN / 2; i++) {
				const uint16_t *hash_as_halfwords =
					(const uint16_t *)BL_STORAGE->key_data[n].hash;
				uint16_t halfword = bl_storage_otp_halfword_read(
					(uint32_t)&hash_as_halfwords[i]);
				if (halfword == 0xFFFF) {
					return -EHASHFF;
				}
			}
		}
	}
	return 0;
}

/**
 * Copies @p src into @p dst. Reads from @p src are done 32 bits at a
 * time. Writes to @p dst are done a byte at a time.
 *
 * @param[out] dst destination buffer.
 * @param[in] src source buffer in OTP. Must be 4-byte-aligned.
 * @param[in] size number of *bytes* in src to copy into dst. Must be divisible by 4.
 */
static void otp_copy32(uint8_t *restrict dst, uint32_t volatile * restrict src,
				   size_t size)
{
	for (int i = 0; i < size / 4; i++) {
		/* OTP is in UICR */
		uint32_t val = bl_storage_word_read((uint32_t)(src + i));

		for (int j = 0; j < 4; j++) {
			dst[i * 4 + j] = (val >> 8 * j) & 0xFF;
		}
	}
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

	if (key_is_valid(key_idx)) {
		/* Write if not already written. */
		bl_storage_word_write((uint32_t)invalidation_token, INVALID_WRITE_VAL);
	}
}

/** Get the counter_collection data structure in the provision data. */
const struct counter_collection *get_counter_collection(void)
{
	const struct counter_collection *collection = (struct counter_collection *)
		&BL_STORAGE->key_data[num_public_keys_read()];
	return bl_storage_otp_halfword_read((uint32_t)&collection->type) == TYPE_COUNTERS
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

	for (size_t i = 0; i < bl_storage_otp_halfword_read(
		(uint32_t)&counters->num_counters); i++) {
		uint16_t num_slots = bl_storage_otp_halfword_read(
					(uint32_t)&current->num_counter_slots);

		if (bl_storage_otp_halfword_read((uint32_t)&current->description) == description) {
			return current;
		}

		current = (const struct monotonic_counter *)
					&current->counter_slots[num_slots];
	}
	return NULL;
}

int num_monotonic_counter_slots(uint16_t counter_desc, uint16_t *counter_slots)
{
	const struct monotonic_counter *counter = get_counter_struct(counter_desc);

	if (counter == NULL || counter_slots == NULL) {
		return -EINVAL;
	}

	uint16_t num_slots = bl_storage_otp_halfword_read((uint32_t)&counter->num_counter_slots);

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
int get_counter(uint16_t counter_desc, counter_t *counter_value, const counter_t **free_slot)
{
	const counter_t *slots;
	counter_t highest_counter = 0;
	const counter_t *addr = NULL;
	const struct monotonic_counter *counter_obj = get_counter_struct(counter_desc);
	uint16_t num_counter_slots;

	if (counter_obj == NULL || counter_value == NULL) {
		return -EINVAL;
	}

	slots = counter_obj->counter_slots;
	num_counter_slots = bl_storage_otp_halfword_read((uint32_t)&counter_obj->num_counter_slots);

	for (uint16_t i = 0; i < num_counter_slots; i++) {
		counter_t counter = bl_storage_counter_get((uint32_t)&slots[i]);

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

int get_monotonic_counter(uint16_t counter_desc, counter_t *counter_value)
{
	return get_counter(counter_desc, counter_value, NULL);
}

int set_monotonic_counter(uint16_t counter_desc, counter_t new_counter)
{
	const counter_t *next_counter_addr;
	counter_t current_cnt_value;
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

	bl_storage_counter_set((uint32_t)next_counter_addr, new_counter);
	return 0;
}

static lcs_data_t bl_storage_lcs_get(uint32_t address)
{
#if defined(CONFIG_NRFX_NVMC)
	return nrfx_nvmc_otp_halfword_read(address);
#elif defined(CONFIG_NRFX_RRAMC)
	return nrfx_rramc_otp_word_read(index_from_address(address));
#endif
}

static int bl_storage_lcs_set(uint32_t address, lcs_data_t state)
{
#if defined(CONFIG_NRFX_NVMC)
	nrfx_nvmc_halfword_write(address, state);
#elif defined(CONFIG_NRFX_RRAMC)
	bl_storage_word_write(address, state);
#endif
	return 0;
}

/* The OTP is 0xFFFF when erased and, like all flash, can only flip
 * bits from 0 to 1 when erasing. By setting all bits to zero we
 * enforce the correct transitioning of LCS until a full erase of the
 * device.
 */
int read_life_cycle_state(enum lcs *lcs)
{
	if (lcs == NULL) {
		return -EINVAL;
	}

	lcs_data_t provisioning = bl_storage_lcs_get(
		(uint32_t) &BL_STORAGE->lcs.provisioning);
	lcs_data_t secure = bl_storage_lcs_get((uint32_t) &BL_STORAGE->lcs.secure);
	lcs_data_t decommissioned = bl_storage_lcs_get(
		(uint32_t) &BL_STORAGE->lcs.decommissioned);

	if (provisioning == STATE_NOT_ENTERED
		&& secure == STATE_NOT_ENTERED
		&& decommissioned == STATE_NOT_ENTERED) {
		*lcs = BL_STORAGE_LCS_ASSEMBLY;
	} else if (provisioning == STATE_ENTERED
			   && secure == STATE_NOT_ENTERED
			   && decommissioned == STATE_NOT_ENTERED) {
		*lcs = BL_STORAGE_LCS_PROVISIONING;
	} else if (provisioning == STATE_ENTERED
			   && secure == STATE_ENTERED
			   && decommissioned == STATE_NOT_ENTERED) {
		*lcs = BL_STORAGE_LCS_SECURED;
	} else if (provisioning == STATE_ENTERED
			   && secure == STATE_ENTERED
			   && decommissioned == STATE_ENTERED) {
		*lcs = BL_STORAGE_LCS_DECOMMISSIONED;
	} else {
		/* To reach this the OTP must be corrupted or reading failed */
		return -EREADLCS;
	}

	return 0;
}

int update_life_cycle_state(enum lcs next_lcs)
{
	int err;
	enum lcs current_lcs = 0;

	if (next_lcs == BL_STORAGE_LCS_UNKNOWN) {
		return -EINVALIDLCS;
	}

	err = read_life_cycle_state(&current_lcs);
	if (err != 0) {
		return err;
	}

	if (next_lcs < current_lcs) {
		/* It is only possible to transition into a higher state */
		return -EINVALIDLCS;
	}

	if (next_lcs == current_lcs) {
		/* The same LCS is a valid argument, but nothing to do so return success */
		return 0;
	}

	/* As the device starts in ASSEMBLY, it is not possible to write it */
	if (current_lcs == BL_STORAGE_LCS_ASSEMBLY && next_lcs == BL_STORAGE_LCS_PROVISIONING) {
		return bl_storage_lcs_set((uint32_t)&BL_STORAGE->lcs.provisioning, STATE_ENTERED);
	}

	if (current_lcs == BL_STORAGE_LCS_PROVISIONING && next_lcs == BL_STORAGE_LCS_SECURED) {
		return bl_storage_lcs_set((uint32_t)&BL_STORAGE->lcs.secure, STATE_ENTERED);
	}

	if (current_lcs == BL_STORAGE_LCS_SECURED && next_lcs == BL_STORAGE_LCS_DECOMMISSIONED) {
		return bl_storage_lcs_set((uint32_t)&BL_STORAGE->lcs.decommissioned, STATE_ENTERED);
	}

	/* This will be the case if any invalid transition is tried */
	return -EINVALIDLCS;
}

#if defined(CONFIG_BUILD_WITH_TFM)

void read_implementation_id_from_otp(uint8_t *buf)
{
	if (buf == NULL) {
		return;
	}

	otp_copy32(buf, (uint32_t *)&BL_STORAGE->implementation_id,
		   BL_STORAGE_IMPLEMENTATION_ID_SIZE);
}

#endif
