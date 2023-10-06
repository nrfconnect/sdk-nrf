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

/* Counter used by NSIB to check the firmware version */
#define BL_MONOTONIC_COUNTERS_DESC_NSIB 0x1

/* Counter used by MCUBOOT to check the firmware version. Suffixed
 * with ID0 as we might support checking the version of multiple
 * images in the future.
 */
#define BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0 0x2

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
 *  struct counter_collection.
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
 * @return Number of public key data slots.
 */
uint32_t num_public_keys_read(void);

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
 * @param[in] src source buffer in OTP. Must be 4-byte-aligned.
 * @param[in] size number of *bytes* in src to copy into dst. Must be divisible by 4.
 */
NRFX_STATIC_INLINE void otp_copy32(uint8_t *restrict dst, uint32_t volatile * restrict src,
				   size_t size)
{
	for (int i = 0; i < size / 4; i++) {
		/* OTP is in UICR */
		uint32_t val = nrfx_nvmc_uicr_word_read(src + i);

		for (int j = 0; j < 4; j++) {
			dst[i * 4 + j] = (val >> 8 * j) & 0xFF;
		}
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

	otp_copy32(buf, (uint32_t *)&BL_STORAGE->implementation_id,
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

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_STORAGE_H_ */
