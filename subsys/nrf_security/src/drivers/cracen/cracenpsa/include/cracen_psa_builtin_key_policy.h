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
	psa_key_usage_t usage;             /**< Key usage intention. */
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
	mbedtls_key_owner_id_t owner;           /**< Key owner ID. */
	psa_drv_slot_number_t key_slot_start;   /**< Start slot number (inclusive). */
	psa_drv_slot_number_t key_slot_end;     /**< End slot number (inclusive). */
	cracen_kmu_entry_type_t kmu_entry_type; /**< Entry type. */
} cracen_builtin_kmu_key_policy_t;

/** @brief Check what a user is allowed to do with an IKG key.
 *
 * @param[in] attributes Key attributes.
 *
 * @return The allowed usage for the given key and user.
 */
psa_key_usage_t cracen_ikg_key_user_get_usage(const psa_key_attributes_t *attributes);

#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
/** @brief Check if a user is allowed to access a KMU key.
 *
 * @param[in] attributes Key attributes.
 *
 * @return Whether the user is allowed to access the key.
 */
bool cracen_kmu_key_user_allowed(const psa_key_attributes_t *attributes);
#endif

#else /* __NRF_TFM__ */

static inline psa_key_usage_t cracen_ikg_key_user_get_usage(const psa_key_attributes_t *attributes)
{
	psa_key_id_t key_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes));

	return (key_id == CRACEN_BUILTIN_IDENTITY_KEY_ID)
		? (PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH |
		   PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH)
		: (PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_VERIFY_DERIVATION);
}

#if defined(PSA_NEED_CRACEN_KMU_DRIVER)
static inline bool cracen_kmu_key_user_allowed(const psa_key_attributes_t *attributes)
{
	(void)attributes;
	return true;
}
#endif

#endif /* __NRF_TFM__ */

/** @} */

#endif /* CRACEN_PSA_BUILTIN_KEY_POLICY_H */
