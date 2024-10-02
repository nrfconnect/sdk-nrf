/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_STORAGE_H_
#define BL_STORAGE_H_

#include <string.h>
#include <zephyr/types.h>
#include <drivers/nrfx_common.h>
#include <nrfx_nvmc.h>
#include <errno.h>
#include <pm_config.h>


#ifdef __cplusplus
extern "C" {
#endif

#define EHASHFF 113 /* A hash contains too many 0xFs. */
#define EREADLCS 114 /* LCS field of OTP is in an invalid state */
#define EINVALIDLCS 115 /* Invalid LCS*/

/* We truncate the 32 byte sha256 down to 16 bytes before storing it */
#define SB_PUBLIC_KEY_HASH_LEN 16

/* Supported collection types. */
enum collection_type {
	BL_COLLECTION_TYPE_MONOTONIC_COUNTERS = 1,
	BL_COLLECTION_TYPE_VARIABLE_DATA = 0x9312,
};

/* Counter used by NSIB to check the firmware version */
#define BL_MONOTONIC_COUNTERS_DESC_NSIB 0x1

/* Counter used by MCUBOOT to check the firmware version. Suffixed
 * with ID0 as we might support checking the version of multiple
 * images in the future.
 */
#define BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0 0x2

struct monotonic_counter {
	/* Counter description. What the counter is used for. See
	 * BL_MONOTONIC_COUNTERS_DESC_x.
	 */
	uint16_t description;
	/* Number of entries in 'counter_slots' list. */
	uint16_t num_counter_slots;
	uint16_t counter_slots[1];
};

/** Common part for all collections. */
struct collection {
	uint16_t type;
	uint16_t count;
};

/** The second data structure in the provision page. It has unknown length since
 *  'counters' is repeated. Note that each entry in counters also has unknown
 *  length, and each entry can have different length from the others, so the
 *  entries beyond the first cannot be accessed via array indices.
 */
struct counter_collection {
	struct collection collection;  /* Type must be BL_COLLECTION_TYPE_MONOTONIC_COUNTERS */
	struct monotonic_counter counters[1];
};

/* Variable data types. */
enum variable_data_type {
	BL_VARIABLE_DATA_TYPE_VERIFICATION_SERVICE_URL = 0x1,
	BL_VARIABLE_DATA_TYPE_PSA_CERTIFICATION_REFERENCE = 0x2,
	BL_VARIABLE_DATA_TYPE_PROFILE_DEFINITION = 0x3,
};
struct variable_data {
	uint8_t type;
	uint8_t length;
	uint8_t data[1];
};

/* The third data structure in the provision page. It has unknown length since
 * 'variable_data' is repeated. The collection starts immediately after the
 * counter collection. As the counter collection has unknown length, the start
 * of the variable data collection must be calculated dynamically. Similarly,
 * the entries in the variable data collection have unknown length, so they
 * cannot be accessed via array indices.
 */
struct variable_data_collection {
	struct collection collection; /* Type must be BL_COLLECTION_TYPE_VARIABLE_DATA */
	struct variable_data variable_data[1];
};

/** Storage for the PRoT Security Lifecycle state, that consists of 4 states:
 *  - Device assembly and test
 *  - PRoT Provisioning
 *  - Secured
 *  - Decommissioned
 *  These states are transitioned top down during the life time of a device.
 *  The Device assembly and test state is implicitly defined by checking if
 *  the provisioning state wasn't entered yet.
 *  This works as ASSEMBLY implies the OTP to be erased.
 */
struct life_cycle_state_data {
	uint16_t provisioning;
	uint16_t secure;
	/* Pad to end the alignment at a 4-byte boundary as the UICR->OTP
	 * only supports 4-byte reads. We place the reserved padding in
	 * the middle of the struct in case we ever need to support
	 * another state.
	 */
	uint16_t reserved_for_padding;
	uint16_t decommissioned;
};

/** The first data structure in the bootloader storage. It has unknown length
 *  since 'key_data' is repeated. This data structure is immediately followed by
 *  struct counter_collection, which is then followed by struct variable_data_collection.
 */
struct bl_storage_data {
	/* NB: When placed in OTP, reads must be 4 bytes and 4 byte aligned */
	struct life_cycle_state_data lcs;
	uint8_t implementation_id[32];
	uint32_t s0_address;
	uint32_t s1_address;
	uint32_t num_public_keys; /* Number of entries in 'key_data' list. */
	struct {
		uint32_t valid;
		uint8_t hash[SB_PUBLIC_KEY_HASH_LEN];
	} key_data[1];

	/* Monotonic counters:
	 * uint16_t type;
	 * uint16_t count;
	 * struct {
	 *	uint16_t description;
	 *	uint16_t num_counter_slots;
	 *	uint16_t counter_slots[1];
	 * } counters[1];
	 */

	/* Variable data:
	 * uint16_t type;
	 * uint16_t count;
	 * struct {
	 *	uint8_t type;
	 *	uint8_t length;
	 *	uint8_t data[1];
	 * } variable_data[1];
	 * uint8_t padding[1];  // Padding to align to 4 bytes
	 */
};

#define BL_STORAGE ((const volatile struct bl_storage_data *)(PM_PROVISION_ADDRESS))

/* This must be 32 bytes according to the IETF PSA token specification */
#define BL_STORAGE_IMPLEMENTATION_ID_SIZE 32

/** @defgroup bl_storage Bootloader storage (protected data).
 * @{
 */

/**
 * @brief Function for reading address of slot 0.
 *
 * @return Address of slot 0.
 */
uint32_t s0_address_read(void);

/**
 * @brief Function for reading address of slot 1.
 *
 * @return Address of slot 1.
 */
uint32_t s1_address_read(void);

/**
 * @brief Function for reading number of public key data slots.
 *
 * @retval 0         if all keys are ok.
 * @retval -EHASHFF  if one or more keys contains an aligned 0xFFFF.
 */
int verify_public_keys(void);

/**
 * @brief Function for reading public key hashes.
 *
 * @param[in]  key_idx  Index of key.
 * @param[out] p_buf    Pointer to where the hash will be written.
 * The buffer must be at least SB_PUBLIC_KEY_HASH_LEN bytes large.
 * @return Number of successfully written bytes to p_buf.
 * @retval -EINVAL  Key has been invalidated.
 * @retval -EFAULT  key_idx is too large. There is no key with that index.
 */
int public_key_data_read(uint32_t key_idx, uint8_t *p_buf);

/**
 * @brief Function for invalidating a public key.
 *
 * The public key will no longer be returned by @ref public_key_data_read.
 *
 * @param[in]  key_idx  Index of key.
 */
void invalidate_public_key(uint32_t key_idx);

/**
 * @brief Get the number of monotonic counter slots.
 *
 * @param[in]   counter_desc  Counter description.
 * @param[out]  counter_slots Number of slots occupied by the counter.
 *
 * @retval 0        Success
 * @retval -EINVAL  Cannot find counters with description @p counter_desc or the pointer to
 *                  @p counter_slots is NULL.
 */
int num_monotonic_counter_slots(uint16_t counter_desc, uint16_t *counter_slots);

/**
 * @brief Get the current HW monotonic counter.
 *
 * @param[in]  counter_desc  Counter description.
 * @param[out] counter_value The value of the current counter.
 *
 * @retval 0        Success
 * @retval -EINVAL  Cannot find counters with description @p counter_desc or the pointer to
 *                  @p counter_value is NULL.
 */
int get_monotonic_counter(uint16_t counter_desc, uint16_t *counter_value);

/**
 * @brief Set the current HW monotonic counter.
 *
 * @note FYI for users looking at the values directly in flash:
 *       Values are stored with their bits flipped. This is to squeeze one more
 *       value out of the counter.
 *
 * @param[in]  counter_desc Counter description.
 * @param[in]  new_counter  The new counter value. Must be larger than the
 *                          current value.
 *
 * @retval 0        The counter was updated successfully.
 * @retval -EINVAL  @p new_counter is invalid (must be larger than current
 *                  counter, and cannot be 0xFFFF).
 * @retval -ENOMEM  There are no more free counter slots (see
 *                  @kconfig{CONFIG_SB_NUM_VER_COUNTER_SLOTS}).
 */
int set_monotonic_counter(uint16_t counter_desc, uint16_t new_counter);

/**
 * @brief Function for reading number of public key data slots.
 *
 * @return Number of public key data slots.
 */
NRFX_STATIC_INLINE uint32_t num_public_keys_read(void)
{
	return nrfx_nvmc_uicr_word_read(&BL_STORAGE->num_public_keys);
}

/**
 * @brief Get the first collection after the public keys.
 *
 * @return Pointer to the first collection.
 */
NRFX_STATIC_INLINE const struct collection *get_first_collection(void)
{
	return (const struct collection *)&BL_STORAGE->key_data[num_public_keys_read()];
}

/**
 * @brief Get the collection type.
 *
 * @param[in] collection Pointer to the collection.
 *
 * @return Collection type.
 */
NRFX_STATIC_INLINE uint16_t get_collection_type(const struct collection *collection)
{
	return nrfx_nvmc_otp_halfword_read((uint32_t)&collection->type);
}

/**
 * @brief The PSA life cycle states a device can be in.
 *
 * The LCS can be only transitioned in the order they are defined here.
 */
enum lcs {
	BL_STORAGE_LCS_UNKNOWN = 0,
	BL_STORAGE_LCS_ASSEMBLY = 1,
	BL_STORAGE_LCS_PROVISIONING = 2,
	BL_STORAGE_LCS_SECURED = 3,
	BL_STORAGE_LCS_DECOMMISSIONED = 4,
};

/**
 * Copies @p src into @p dst. Reads from @p src are done 32 bits at a
 * time. Writes to @p dst are done a byte at a time.
 *
 * @param[out] dst destination buffer.
 * @param[in] src source buffer in OTP.
 * @param[in] size number of *bytes* in src to copy into dst.
 */
NRFX_STATIC_INLINE void otp_copy(uint8_t *restrict dst, uint8_t volatile * restrict src,
				 size_t size)
{
	size_t copied = 0;			/* Bytes copied. */
	uint8_t src_offset = (uint32_t)src % 4; /* Align the source address to 32-bits. */

	uint32_t val;	   /* 32-bit value read from the OTP. */
	uint8_t copy_size; /* Number of bytes to copy. */

	while (copied < size) {

		/* Read 32-bits. */
		val = nrfx_nvmc_uicr_word_read((uint32_t *)(src + copied - src_offset));

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

/**
 * Read the implementation id from OTP and copy it into a given buffer.
 *
 * @param[out] buf Buffer that has at least BL_STORAGE_IMPLEMENTATION_ID_SIZE bytes
 */
NRFX_STATIC_INLINE void read_implementation_id_from_otp(uint8_t *buf)
{
	if (buf == NULL) {
		return;
	}

	otp_copy(buf, (uint8_t *)&BL_STORAGE->implementation_id,
		   BL_STORAGE_IMPLEMENTATION_ID_SIZE);
}

/* The OTP is 0xFFFF when erased and, like all flash, can only flip
 * bits from 0 to 1 when erasing. By setting all bits to zero we
 * enforce the correct transitioning of LCS until a full erase of the
 * device.
 */
#define STATE_ENTERED 0x0000
#define STATE_NOT_ENTERED 0xFFFF

/* The bl_storage functions below are static inline in the header file
 * so that TF-M (that does not include bl_storage.c) can also have
 * access to them.
 * This is a temporary solution until TF-M has access to NSIB functions.
 */

/**
 * @brief Read the current life cycle state the device is in from OTP,
 *
 * @param[out] lcs Will be set to the current LCS the device is in
 *
 * @retval 0            The LCS read was successful.
 * @retval -EREADLCS    Error on reading from OTP or invalid OTP content.
 */
NRFX_STATIC_INLINE int read_life_cycle_state(enum lcs *lcs)
{
	if (lcs == NULL) {
		return -EINVAL;
	}

	uint16_t provisioning = nrfx_nvmc_otp_halfword_read(
		(uint32_t) &BL_STORAGE->lcs.provisioning);
	uint16_t secure = nrfx_nvmc_otp_halfword_read((uint32_t) &BL_STORAGE->lcs.secure);
	uint16_t decommissioned = nrfx_nvmc_otp_halfword_read(
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

/**
 * @brief Update the life cycle state in OTP.
 *
 * @param[in] next_lcs Must be the same or the successor state of the current
 *                     one.
 *
 * @retval 0            Success.
 * @retval -EREADLCS    Reading the current state failed.
 * @retval -EINVALIDLCS Invalid next state.
 */
NRFX_STATIC_INLINE int update_life_cycle_state(enum lcs next_lcs)
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
		/* Is is only possible to transition into a higher state */
		return -EINVALIDLCS;
	}

	if (next_lcs == current_lcs) {
		/* The same LCS is a valid argument, but nothing to do so return success */
		return 0;
	}

	/* As the device starts in ASSEMBLY, it is not possible to write it */
	if (current_lcs == BL_STORAGE_LCS_ASSEMBLY && next_lcs == BL_STORAGE_LCS_PROVISIONING) {
		nrfx_nvmc_halfword_write((uint32_t)&BL_STORAGE->lcs.provisioning, STATE_ENTERED);
		return 0;
	}

	if (current_lcs == BL_STORAGE_LCS_PROVISIONING && next_lcs == BL_STORAGE_LCS_SECURED) {
		nrfx_nvmc_halfword_write((uint32_t)&BL_STORAGE->lcs.secure, STATE_ENTERED);
		return 0;
	}

	if (current_lcs == BL_STORAGE_LCS_SECURED && next_lcs == BL_STORAGE_LCS_DECOMMISSIONED) {
		nrfx_nvmc_halfword_write((uint32_t)&BL_STORAGE->lcs.decommissioned, STATE_ENTERED);
		return 0;
	}

	/* This will be the case if any invalid transition is tried */
	return -EINVALIDLCS;
}

#if CONFIG_BUILD_WITH_TFM

static uint32_t get_monotonic_counter_collection_size(const struct counter_collection *collection)
{
	/* Add only the constant part of the counter_collection. */
	uint32_t size = sizeof(struct collection);
	uint16_t num_counters =
		nrfx_nvmc_otp_halfword_read((uint32_t)&collection->collection.count);
	const struct monotonic_counter *counter = collection->counters;

	for (int i = 0; i < num_counters; i++) {
		/* Add only the constant part of the monotonic_counter. */
		size += sizeof(struct monotonic_counter) - sizeof(uint16_t);

		uint16_t num_slots =
			nrfx_nvmc_otp_halfword_read((uint32_t)&counter->num_counter_slots);
		size += (num_slots * sizeof(uint16_t));

		/* Move to the next monotonic counter. */
		counter = (const struct monotonic_counter *)&counter->counter_slots[num_slots];
	}

	return size;
}

static const struct variable_data_collection *get_variable_data_collection(void)
{
	/* We expect to find variable data after the monotonic counters. */
	const struct collection *collection = get_first_collection();

	if (get_collection_type(collection) == BL_COLLECTION_TYPE_MONOTONIC_COUNTERS) {

		/* Advance to next collection. */
		collection = (const struct collection *)((uint8_t *)collection +
							 get_monotonic_counter_collection_size(
								 (const struct counter_collection *)
									 collection));

		/* Verify that we found variable collection. */
		return get_collection_type(collection) == BL_COLLECTION_TYPE_VARIABLE_DATA
			       ? (const struct variable_data_collection *)collection
			       : NULL;

	} else if (get_collection_type(collection) == BL_COLLECTION_TYPE_VARIABLE_DATA) {
		/* Bit of a special scenario where monotonic counters are not present. */
		return (const struct variable_data_collection *)collection;
	}

	return NULL;
}

/**
 * @brief Read variable data from OTP.
 *
 * Variable data starts with variable data collection ID, followed by amount of variable data
 * entries and the variable data entries themselves.
 * [Collection ID][Variable count][Type][Variable data length][Variable data][Type]...
 *  2 bytes        2 bytes         1 byte 1 byte                0-255 bytes
 *
 * @note If data is not found, function does not fail. Instead, 0 length is returned.
 *
 * @param[in] data_type Type of the variable data to read.
 * @param[out] buf      Buffer to store the variable data.
 * @param[in,out] buf_len  On input, the size of the buffer. On output, the length of the data.
 *
 * @retval 0            Variable data read successfully, or not found.
 * @retval -EINVAL      No buffer provided.
 * @retval -ENOMEM      Buffer too small.
 */
int read_variable_data(enum variable_data_type data_type, uint8_t *buf, uint32_t *buf_len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	const struct variable_data_collection *collection = get_variable_data_collection();

	if (collection == NULL) {
		/* Variable data collection does not necessarily exist. Exit gracefully. */
		*buf_len = 0;
		return 0;
	}

	const struct variable_data *variable_data = collection->variable_data;
	const uint16_t count = nrfx_nvmc_otp_halfword_read((uint32_t)&collection->collection.count);
	uint8_t type;
	uint8_t length;

	/* Loop through all variable data entries. */
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
#endif

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_STORAGE_H_ */
