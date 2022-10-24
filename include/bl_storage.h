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

#ifdef __cplusplus
extern "C" {
#endif


#if defined(CONFIG_IS_SECURE_BOOTLOADER) || defined(CONFIG_SECURE_BOOT)
#include <pm_config.h>
#define PROVISION_PARTITION_ADDRESS PM_PROVISION_ADDRESS
#else
#define PROVISION_PARTITION_ADDRESS CONFIG_SECURE_BOOT_STORAGE_START_ADDRESS
#endif

#define EHASHFF 113 /* A hash contains too many 0xFs. */
#define EREADLCS 114 /* LCS field of OTP is in an invalid state */
#define EINVALIDLCS 115 /* Invalid LCS*/

/* We truncate the 32 byte sha256 down to 16 bytes before storing it */
#define SB_PUBLIC_KEY_HASH_LEN 16

/** Storage for the PRoT Security Lifecycle state, that consists of 4 states:
 *  - Device assembly and test
 *  - PRoT Provisioning
 *  - Secured
 *  - Decommissioned
 *  These states are transitioned top down during the life time of a device.
 *  The Device assembly and test state is implicitly defined by checking if
 *  the provisioning state wasn't entered yet.
 *  This works as ASSEMBLY implies the OTP be erased.
 */
struct life_cycle_state_data {
	uint16_t provisioning;
	uint16_t secure;
	/* Pad to end the alignment at a 4-byte boundary as the UICR->OTP
	 * only supports 4-byte reads
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
	uint32_t s0_address;
	uint32_t s1_address;
	uint32_t num_public_keys; /* Number of entries in 'key_data' list. */
#ifdef CONFIG_SB_IMPLEMENTATION_ID
	uint8_t implementation_id[32];
#endif
	struct {
		uint32_t valid;
		uint8_t hash[SB_PUBLIC_KEY_HASH_LEN];
	} key_data[1];
};

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
 * @retval 0         if all keys are ok
 * @retval -EHASHFF  if one or more keys contains an aligned 0xFFFF.
 */
int verify_public_keys(void);

/**
 * @brief Function for reading public key data.
 *
 * @param[in]  key_idx  Index of key.
 * @param[out] p_buf    Pointer to area where key data will be stored.
 * The buffer must be at least SB_PUBLIC_KEY_HASH_LEN bytes large.
 *
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
 * @return The number of slots. If the provision page does not contain the
 *         information, 0 is returned.
 */
uint16_t num_monotonic_counter_slots(void);

/**
 * @brief Get the current HW monotonic counter.
 *
 * @return The current value of the counter.
 */
uint16_t get_monotonic_counter(void);

/**
 * @brief Set the current HW monotonic counter.
 *
 * @note FYI for users looking at the values directly in flash:
 *       Values are stored with their bits flipped. This is to squeeze one more
 *       value out of the counter.
 *
 * @param[in]  new_counter  The new counter value. Must be larger than the
 *                          current value.
 *
 * @retval 0        The counter was updated successfully.
 * @retval -EINVAL  @p new_counter is invalid (must be larger than current
 *                  counter, and cannot be 0xFFFF).
 * @retval -ENOMEM  There are no more free counter slots (see
 *                  @kconfig{CONFIG_SB_NUM_VER_COUNTER_SLOTS}).
 */
int set_monotonic_counter(uint16_t new_counter);

/**
 * @brief The PSA life cycle states a device can be in.
 *
 * The LCS can be only transitioned in the order they are defined here.
 */
enum lcs {
	UNKNOWN = 0,
	ASSEMBLY = 1,
	PROVISION = 2,
	SECURE = 3,
	DECOMMISSIONED = 4,
};

/**
 * @brief Update the life cycle state in OTP,
 *
 * If the given next state is not a not allowed transition the function
 * will return -EINVALIDLCS
 *
 * @param[in] next_lcs Must be the same or the successor state of the current
 *                     one.
 *
 * @retval 0            The LSC was update successfully
 * @retval -EREADLCS    Error on reading the current state
 * @retval -EINVALIDLCS Invalid next state
 */
int update_life_cycle_state(enum lcs next_lcs);

/**
 * @brief Read the current life cycle state the device is in from OTP,
 *
 * @param[out] lcs Will be set to the current LCS the device is in
 *
 * @retval 0            The LSC read was successful
 * @retval -EREADLCS    Error on reading from OTP or invalid OTP content
 */
int read_life_cycle_state(enum lcs *lcs);

/**
 * Copies @p src into @p dst. Reads from @p src are done 32 bits at a
 * time. Writes to @p dst are done a byte at a time.
 *
 * This ensures UICR->OTP's word-aligned and word-sized read
 * requirement is met and also allows the destination buffer to be
 * free of this requirement.
 *
 * @param dst destination buffer
 * @param src source buffer
 * @param size number of *bytes* in src to copy into dst
 */
NRFX_STATIC_INLINE void otp_copy32(uint8_t *restrict dst, uint32_t volatile *restrict src,
				   size_t size)
{
	for (int i = 0; i < size / 4; i++) {
		uint32_t val = src[i];

		for (int j = 0; j < 4; j++) {
			dst[i * 4 + j] = (val >> 8 * j) & 0xFF;
		}
	}
}

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_STORAGE_H_ */
