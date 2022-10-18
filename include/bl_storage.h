/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_STORAGE_H_
#define BL_STORAGE_H_

#include <zephyr/types.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


#define EHASHFF 113 /* A hash contains too many 0xFs. */
#define EREADLCS 114 /* LCS field of OTP is in an invalid state */
#define EINVALIDLCS 115 /* Invalid LCS*/

/** Storage for the life psa cycle state, that consists of 4 states:
 *  - ASSEMBLY
 *  - PSA RoT Provisioning
 *  - SECURE
 *  - DECOMMISSIONED
 *  These states are transitioned top down during the life time of a device.
 *  Therefore when setting a new LCS we just mark the old state as left, this
 *  way one field less can be used to encode the lcs.
 *  This works as ASSEMBLY implies the OTP be erased except of needed
 *  key material
 */
struct life_cycle_state_data {
	uint16_t left_assembly;
	uint16_t left_provisioning;
	uint16_t left_secure;
};

/** The first data structure in the bootloader storage. It has unknown length
 *  since 'key_data' is repeated. This data structure is immediately followed by
 *  struct counter_collection.
 */
struct bl_storage_data {
	uint32_t s0_address;
	uint32_t s1_address;
	struct life_cycle_state_data lcs;
	uint32_t num_public_keys; /* Number of entries in 'key_data' list. */
	uint8_t implementation_id[32];
	struct {
		uint32_t valid;
		uint8_t hash[CONFIG_SB_PUBLIC_KEY_HASH_LEN];
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
 * @param[in]  buf_size Size of buffer, in bytes, provided in p_buf.
 *
 * @return Number of bytes written to p_buf is successful.
 * @retval -EINVAL  Key has been invalidated.
 * @retval -ENOMEM  The provided buffer is too small.
 * @retval -EFAULT  key_idx is too large. There is no key with that index.
 */
int public_key_data_read(uint32_t key_idx, uint8_t *p_buf, size_t buf_size);

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
 * The can be only transitioned in the order they are defined here.
 */
enum lcs {
	UNKOWN = 0,
	ASSEMBLY = 1,
	PROVISION = 2,
	SECURE = 3,
	DECOMMISSIONED = 4,
};

typedef enum lcs lcs_t;

/**
 * @brief Write a life cycle state to OTP,
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
int write_life_cycle_state(lcs_t next_lcs);

/**
 * @brief Read the current life cycle state the device is in from OTP,
 *
 * @param[out] lcs Will be set to the current LCS the device is in
 *
 * @retval 0            The LSC read was successful
 * @retval -EREADLCS    Error on reading from OTP or invalid OTP content
 */
int read_life_cycle_state(lcs_t *lcs);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_STORAGE_H_ */
