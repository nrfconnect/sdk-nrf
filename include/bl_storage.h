/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_STORAGE_H_
#define BL_STORAGE_H_

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/autoconf.h>
#include <nrfx.h>
#if defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#elif defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#else
#error "No NRFX storage technology supported backend selected"
#endif
#include <errno.h>
#include <pm_config.h>


#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NRFX_NVMC)
typedef uint16_t counter_t;
typedef uint16_t lcs_data_t;
typedef uint16_t lcs_reserved_t;
#elif defined(CONFIG_NRFX_RRAMC)
/* nRF54L15 only supports word writes */
typedef uint32_t counter_t;
typedef uint32_t lcs_data_t;
typedef uint32_t lcs_reserved_t;
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
	lcs_data_t provisioning;
	lcs_data_t secure;
	/* Pad to end the alignment at a 4-byte boundary as some devices
	 * are only supporting 4-byte UICR->OTP reads. We place the reserved
	 * padding in the middle of the struct in case we ever need to support
	 * another state.
	 */
	lcs_reserved_t reserved_for_padding;
	lcs_data_t decommissioned;
};

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
	counter_t counter_slots[];
};

/** Common part for all collections. */
struct collection {
	uint16_t type;
	uint16_t count;
};

/** The second data structure in the provision page. It has unknown length since
 *  'counters' is repeated. Note that each entry in counters also has unknown
 *  length, and each entry can have different length from the others, so the
 *  entries beyond the first cannot be accessed through array indices.
 */
struct counter_collection {
	struct collection collection;  /* Type must be BL_COLLECTION_TYPE_MONOTONIC_COUNTERS */
	struct monotonic_counter counters[];
};

/* Variable data types. */
enum variable_data_type {
	BL_VARIABLE_DATA_TYPE_PSA_CERTIFICATION_REFERENCE = 0x1
};
struct variable_data {
	uint8_t type;
	uint8_t length;
	uint8_t data[];
};

/* The third data structure in the provision page. It has unknown length since
 * 'variable_data' is repeated. The collection starts immediately after the
 * counter collection. As the counter collection has unknown length, the start
 * of the variable data collection must be calculated dynamically. Similarly,
 * the entries in the variable data collection have unknown length, so they
 * cannot be accessed through array indices.
 */
struct variable_data_collection {
	struct collection collection; /* Type must be BL_COLLECTION_TYPE_VARIABLE_DATA */
	struct variable_data variable_data[];
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
	} key_data[];

	/* Monotonic counter collection:
	 * uint16_t type;
	 * uint16_t count;
	 * struct {
	 *	uint16_t description;
	 *	uint16_t num_counter_slots;
	 *	counter_t counter_slots[];
	 * } counters[];
	 */

	/* Variable data collection:
	 * uint16_t type;
	 * uint16_t count;
	 * struct {
	 *	uint8_t type;
	 *	uint8_t length;
	 *	uint8_t data[];
	 * } variable_data[];
	 * uint8_t padding[];  // Padding to align to 4 bytes
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
 * @return Number of public key data slots.
 */
uint32_t num_public_keys_read(void);

/**
 * @brief Function for verifying public keys.
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
int get_monotonic_counter(uint16_t counter_desc, counter_t *counter_value);

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
 *                  @kconfig{SB_CONFIG_SECURE_BOOT_NUM_VER_COUNTER_SLOTS}).
 */
int set_monotonic_counter(uint16_t counter_desc, counter_t new_counter);

/**
 * @brief Checks whether it is possible to update the monotonic counter
 *        to a new value.
 *
 * @param[in]  counter_desc Counter description.
 *
 * @retval 0        The counter was updated successfully.
 * @retval -EINVAL  @p counter_desc is invalid.
 * @retval -ENOMEM  There are no more free counter slots (see
 *                  @kconfig{SB_CONFIG_SECURE_BOOT_NUM_VER_COUNTER_SLOTS}).
 */
int is_monotonic_counter_update_possible(uint16_t counter_desc);

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
 * @brief Read the current life cycle state the device is in from OTP,
 *
 * @param[out] lcs Will be set to the current LCS the device is in
 *
 * @retval 0            The LCS read was successful.
 * @retval -EREADLCS    Error on reading from OTP or invalid OTP content.
 */
int read_life_cycle_state(enum lcs *lcs);

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
int update_life_cycle_state(enum lcs next_lcs);

/**
 * Read the implementation ID from OTP and copy it into a given buffer.
 *
 * @param[out] buf Buffer that has at least BL_STORAGE_IMPLEMENTATION_ID_SIZE bytes
 */
void read_implementation_id_from_otp(uint8_t *buf);

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
int read_variable_data(enum variable_data_type data_type, uint8_t *buf, uint32_t *buf_len);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_STORAGE_H_ */
