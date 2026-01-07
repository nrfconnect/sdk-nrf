/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_builtin_key_policy
 * @{
 * @brief Built-in key policy definitions for CRACEN PSA driver.
 *
 * @note These definitions are shared between the public driver API and internal
 *       implementation.
 *
 * This module provides definitions and functions for managing built-in key
 * policies, shared between the public driver API and internal implementation.
 */

#ifndef CRACEN_PSA_BUILTIN_KEY_POLICY_H
#define CRACEN_PSA_BUILTIN_KEY_POLICY_H

#include <psa/crypto.h>
#include <psa/crypto_values.h>

#if defined(__NRF_TFM__)

/** @brief Built-in IKG key policy structure. */
typedef struct {
	mbedtls_key_owner_id_t owner;      /**< Key owner ID. */
	psa_drv_slot_number_t key_slot;    /**< Key slot number. */
} cracen_builtin_ikg_key_policy_t;

/** @brief KMU entry type. */
typedef enum {
	KMU_ENTRY_SLOT_SINGLE,  /**< Single slot entry. */
	KMU_ENTRY_SLOT_RANGE,   /**< Range of slots entry. */
} cracen_kmu_entry_type_t;

/** @brief Built-in KMU key policy structure.
 *
 * When defining a range of KMU slots, both the start and end slot numbers
 * are inclusive.
 */
typedef struct {
	mbedtls_key_owner_id_t owner;         /**< Key owner ID. */
	psa_drv_slot_number_t key_slot_start; /**< Start slot number (inclusive). */
	psa_drv_slot_number_t key_slot_end;   /**< End slot number (inclusive). */
	cracen_kmu_entry_type_t kmu_entry_type; /**< Entry type. */
} cracen_builtin_kmu_key_policy_t;

/** @brief Check if a user is allowed to access a built-in key.
 *
 * @param[in] attributes Key attributes.
 *
 * @retval true  The user is allowed to access the key.
 * @retval false The user is not allowed to access the key.
 */
bool cracen_builtin_key_user_allowed(const psa_key_attributes_t *attributes);

#else

static inline bool cracen_builtin_key_user_allowed(const psa_key_attributes_t *attributes)
{
	(void)attributes;
	return true;
}

#endif /* __NRF_TFM__ */

/** @} */

#endif /* CRACEN_PSA_BUILTIN_KEY_POLICY_H */
