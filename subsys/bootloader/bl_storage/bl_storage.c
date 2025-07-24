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
#include <zephyr/logging/log.h>

#if !defined(CONFIG_BUILD_WITH_TFM)
#include <zephyr/kernel.h>
#endif

LOG_MODULE_REGISTER(bl_storage, CONFIG_SECURE_BOOT_STORAGE_LOG_LEVEL);
//LOG_MODULE_REGISTER(bl_storage, 4);

#define ALIGN_TO_WORD(x) ((uint32_t)x & 0x3)

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

#if defined(CONFIG_BUILD_WITH_TFM)
void tfm_core_panic(void);
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

	if (!ALIGN_TO_WORD(address)) {
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
#if defined(CONFIG_IS_SECURE_BOOTLOADER) || defined(CONFIG_SECURE_BOOT)
uint32_t s0_address_read(void)
{
#ifdef CONFIG_PARTITION_MANAGER_ENABLED
#if defined(CONFIG_SOC_NRF5340_CPUNET)
	return PM_APP_ADDRESS;
#else
	return PM_S0_ADDRESS;
#endif
#else
#error "Not currently supported"
#endif
}

#if !defined(CONFIG_SOC_NRF5340_CPUNET)
uint32_t s1_address_read(void)
{
#ifdef CONFIG_PARTITION_MANAGER_ENABLED
	return PM_S1_ADDRESS;
#else
#error "Not currently supported"
#endif
}
#endif
#endif

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
	tfm_core_panic();
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
 * @param[in] src source buffer in OTP.
 * @param[in] size number of *bytes* in src to copy into dst.
 */
static void otp_copy(uint8_t *restrict dst, const uint8_t volatile * restrict src, size_t size)
{
	size_t copied = 0; /* Bytes copied. */
	uint8_t src_offset = ALIGN_TO_WORD(src); /* Align the src address to 32-bits. */

	uint32_t val;	   /* 32-bit value read from the OTP. */
	uint8_t copy_size; /* Number of bytes to copy. */

	while (copied < size) {
		/* Read 32-bits. */
		val = bl_storage_word_read((uint32_t)(src + copied - src_offset));

		/* Calculate the size to copy. */
		copy_size = sizeof(val) - src_offset;
		if (size - copied < copy_size) {
			copy_size = size - copied;
		}

		/* Copy the data one byte at a time. */
		for (int i = 0; i < copy_size; i++) {
			*dst++ = (val >> (8 * (i + src_offset))) & 0xFF;
		}

		/* Source address is aligned to 32-bits after the first iteration. */
		src_offset = 0;

		copied += copy_size;
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

	otp_copy(p_buf, p_key, SB_PUBLIC_KEY_HASH_LEN);

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

static const uint8_t *get_first_collection(void)
{
	return (const uint8_t *)&BL_STORAGE->key_data[num_public_keys_read()];
}

static const uint16_t get_collection_slots(uint16_t type)
{
	switch (type) {
#if defined(CONFIG_SB_NUM_VER_COUNTER_SLOTS) && CONFIG_SB_NUM_VER_COUNTER_SLOTS > 0
	case BL_MONOTONIC_COUNTERS_DESC_NSIB:
	{
		return CONFIG_SB_NUM_VER_COUNTER_SLOTS;
	}
#endif
#if defined(CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS) && CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS > 0
	case BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0:
	{
		return CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS;
	}
#endif
	default:
	{
		return 0;
	}
	};
}

static const uint16_t get_collection_size(uint16_t type)
{
	return get_collection_slots(type) * sizeof(counter_t);
}

/** Get the counter_collection data structure in the provision data. */
static const uint8_t *get_counter_collection(uint16_t type)
{
	uint16_t i = 0;
	const uint8_t *collection = get_first_collection();

	while (i < type) {
		collection += get_collection_size(i);
		++i;
	}

LOG_ERR("get_counter_collection for %d = %p", type, collection);

	return collection;
}

int num_monotonic_counter_slots(uint16_t counter_desc, uint16_t *counter_slots)
{
        const uint8_t *counters = get_counter_collection(counter_desc);
	const uint16_t num_slots = get_collection_slots(counter_desc);

	if (counters == NULL || counter_slots == NULL) {
		return -EINVAL;
	}

LOG_ERR("slots for %d = %d", counter_desc, num_slots);

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
	counter_t highest_counter = 0;
	const counter_t *addr = NULL;
	const uint8_t *counter_obj = get_counter_collection(counter_desc);
	uint16_t num_counter_slots;

LOG_ERR("counter_obj: %p, %p", counter_obj, counter_value);

	if (counter_obj == NULL || counter_value == NULL) {
		return -EINVAL;
	}

	num_counter_slots = get_collection_slots(counter_desc);
LOG_ERR("slots for %d = %d", counter_desc, num_counter_slots);

	for (uint16_t i = 0; i < num_counter_slots; i++) {
		const counter_t *slot_addr = (counter_t *)(counter_obj + (i * sizeof(counter_t)));
		counter_t counter = bl_storage_counter_get((uint32_t)slot_addr);

//LOG_ERR("read @ 0x%x = %u", slot_addr, counter);

		if (counter == 0) {
			addr = (counter_t *)slot_addr;
			break;
		}

		if (highest_counter < counter) {
			highest_counter = counter;
		}
	}

LOG_ERR("free slot: %p, addr: %p", free_slot, addr);
	if (free_slot != NULL) {
		*free_slot = addr;
	}

LOG_ERR("highest count: %d", highest_counter);
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

	otp_copy(buf, (uint8_t *)&BL_STORAGE->implementation_id,
		   BL_STORAGE_IMPLEMENTATION_ID_SIZE);
}

int read_variable_data(enum variable_data_type data_type, uint8_t *buf, uint32_t *buf_len)
{
	const struct variable_data_collection *collection =
				(const struct variable_data_collection *)get_counter_collection(
						BL_COLLECTION_TYPE_VARIABLE_DATA);
	const struct variable_data *variable_data;
	uint16_t count;
	uint8_t type;
	uint8_t length;

	if (buf == NULL) {
		return -EINVAL;
	}

	/* Loop through all variable data entries. */
	variable_data = collection->variable_data;
	count = bl_storage_otp_halfword_read((uint32_t)&collection->collection.count);
	for (int i = 0; i < count; i++) {
		/* Read the type and length of the variable data. */
		otp_copy((uint8_t *)&type, (uint8_t *)&variable_data->type, sizeof(type));
		otp_copy((uint8_t *)&length, (uint8_t *)&variable_data->length, sizeof(length));

		if (type == data_type) {
			/* Found the requested variable data. */
			if (*buf_len < length) {
				return -ENOMEM;
			}

			/* Copy the variable data into the buffer. */
			otp_copy(buf, (uint8_t *)&variable_data->data, length);
			*buf_len = length;
			return 0;
		}
		/* Move to the next variable data entry. */
		variable_data =
			(const struct variable_data *)((uint8_t *)&variable_data->data + length);
	}

	/* No matching variable data. */
	*buf_len = 0;

	return 0;
}

#endif /* CONFIG_BUILD_WITH_TFM */
