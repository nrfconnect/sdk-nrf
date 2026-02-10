/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_kmu
 * @{
 * @brief Key Management Unit (KMU) support for CRACEN PSA driver.
 *
 * This module provides APIs and macros for managing keys stored in the KMU
 * hardware peripheral, including key slot management, key usage schemes, and
 * key blocking operations.
 *
 * @note Applications must use the PSA Crypto API (psa_* functions) instead
 *       of calling these driver functions directly. This header documents the
 *       KMU driver implementation that is called by the PSA Crypto layer.
 */

#ifndef CRACEN_PSA_KMU_H
#define CRACEN_PSA_KMU_H

#include <psa/crypto.h>

#ifdef CONFIG_BUILD_WITH_TFM
/** @brief A driver-specific representation of a key.
 *
 * Values of this type are used to identify built-in keys.
 */
typedef uint64_t psa_drv_slot_number_t;
#endif

/** @brief Key location identifier for CRACEN KMU.
 *
 * This macro defines the PSA key location value for keys stored in the CRACEN
 * KMU hardware peripheral.
 */
#define PSA_KEY_LOCATION_CRACEN_KMU (PSA_KEY_LOCATION_VENDOR_FLAG | ('N' << 8) | 'K')

/** @brief Construct a PSA key handle for a key stored in KMU.
 *
 * The key ID format is 0x7fffXYZZ where:
 * - X: Key usage scheme (4 bits)
 * - Y: Reserved (0)
 * - Z: KMU slot index (8 bits)
 *
 * @param[in] scheme  Key usage scheme from @ref cracen_kmu_metadata_key_usage_scheme.
 * @param[in] slot_id KMU slot index (0-255).
 *
 * @return PSA key handle value.
 */
#define PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(scheme, slot_id)                                       \
	(0x7fff0000 | ((scheme) << 12) | ((slot_id)&0xff))

/** @brief Retrieve key usage scheme from PSA key ID.
 *
 * @param[in] key_id PSA key ID.
 *
 * @return Key usage scheme value (0-15).
 */
#define CRACEN_PSA_GET_KEY_USAGE_SCHEME(key_id) (((key_id) >> 12) & 0xf)

/** @brief Retrieve KMU slot number from PSA key ID.
 *
 * @param[in] key_id PSA key ID.
 *
 * @return KMU slot number (0-255).
 */
#define CRACEN_PSA_GET_KMU_SLOT(key_id) ((key_id)&0xff)

/** @brief Key usage schemes for KMU metadata.
 *
 * Defines how keys stored in KMU are handled and used by the CRACEN driver.
 */
enum cracen_kmu_metadata_key_usage_scheme {
	/** Keys can only be pushed to CRACEN's protected RAM.
	 *
	 * The keys are not encrypted. Only AES is supported.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED,

	/** CRACEN's IKG seed uses three key slots.
	 *
	 * Pushed to the seed register.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_SEED,

	/** Keys are stored in encrypted form.
	 *
	 * They will be decrypted to the KMU push area for usage.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED,

	/** Keys are not encrypted.
	 *
	 * Pushed to the KMU push area.
	 */
	CRACEN_KMU_KEY_USAGE_SCHEME_RAW
};

/** @brief Retrieve the slot number for a given key handle.
 *
 * @param[in]  key_id      Key handle identifier.
 * @param[out] lifetime    Lifetime for the key.
 * @param[out] slot_number The key's slot number.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST The key does not exist.
 */
psa_status_t cracen_kmu_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				     psa_drv_slot_number_t *slot_number);

/** @brief Block a key in the KMU.
 *
 * This function blocks a key stored in KMU, preventing its use in cryptographic
 * operations.
 *
 * @param[in] key_attr Attributes of the key to block.
 *
 * @note The underlying operation performed is a `BLOCK` on devices that support
 *       it, and a `PUSHBLOCK` on those that do not.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST The key does not exist.
 */
psa_status_t cracen_kmu_block(const psa_key_attributes_t *key_attr);

/** @} */

#endif /* CRACEN_PSA_KMU_H */
