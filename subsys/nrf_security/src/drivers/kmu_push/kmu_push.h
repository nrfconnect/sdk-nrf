/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KMU_PUSH_H
#define KMU_PUSH_H

/** @brief Default mask value indicating no additional behavior */
#define KMU_PUSH_USAGE_MASK_NONE		(0)

/** @brief Mask value to immediately push a KMU slot when importing */
#define KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY	(1 << 0)

/** @brief Mask value to immediately revoke a KMU slot after importing */
#define KMU_PUSH_USAGE_MASK_DESTROY_AFTER	(2 << 0)

/** @brief Key location identifier for KMU push.
 *
 * This macro defines the PSA key location value for keys stored KMU for
 * push operations.
 */
#define PSA_KEY_LOCATION_KMU_PUSH (PSA_KEY_LOCATION_VENDOR_FLAG | ('@' << 8) | 'd')

/** @brief Structure type for KMU push key buffer
 *
 * This structure type is used for KMU push driver which allows
 * an arbitruary key buffer to be pushed to an arbitruary address in
 * KMU-accessible memory location. This structure type is used directly when
 * calling psa_import_key.
 */
typedef struct {
	uint32_t usage_mask;	/**!< Usage mask controlling if imported data shall be pushed and optionally destroyed. */
	uint32_t dest_address; 	/**!< Destination address of the KMU push operation. */
	uint8_t *key_buffer;	/**!< Buffer containing the key to push. */
	size_t buffer_size;	/**!< Size of the buffer containing the key. */
} kmu_push_key_buffer_t;

/** @brief Import a KMU push key.
 *
 * The key_buffer is required to be @ref kmu_push_key_buffer_t. Note that
 * depending on usage_mask metadata in this structure, the KMU push key may be
 * directly pushed and/or destroyed during the call to import the key.
 *
 * @param[in] attributes          Key attributes.
 * @param[in] data                Key data to import.
 * @param[in] data_length         Length of the key data in bytes.
 * @param[out] key_buffer         Buffer to represent the stored key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the imported key in bytes.
 * @param[out] key_bits           Size of the key in bits.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    A wrong lifetime was used (revoked or read-only).
 * @retval PSA_ERROR_INVALID_ARGUMENT The key data is invalid.
 * @retval PSA_ERROR_NOT_PERMITTED    Storing the KMU key is not permitted for given slots.
 * @retval PSA_ERROR_BAD_STATE        The KMU metadata retrieval failed with unknown error.
 */
psa_status_t kmu_push_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				 size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
				 size_t *key_buffer_length, size_t *key_bits);

/** @brief Get the key slot and lifetime for a given key ID.
 *
 * @param[in] key_id       Key ID.
 * @param[out] lifetime    Key lifetime.
 * @param[out] slot_number Key slot number.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key_id does not match a KMU push key.
 * @retval PSA_ERROR_NOT_SUPPORTED    The key lifetime is unsupported.
 * @retval PSA_ERROR_NOT_PERMITTED    The KMU slot is invalid (e.g. revoked or other format)
 * @retval PSA_ERROR_INVALID_HANDLE   The key handle is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST   The key does not exist.
 * @retval PSA_ERROR_BAD_STATE        The KMU metadata retrieval failed with unknown error.
 */
psa_status_t kmu_push_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				   psa_drv_slot_number_t *slot_number);

/** @brief Get the size of the represented KMU push key.
 *
 * @param[in] attributes Key attributes.
 * @param[out] key_size  Size of the key representation in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 */
psa_status_t kmu_push_get_opaque_size(const psa_key_attributes_t *attributes, size_t *key_size);


/** @brief PSA driver API to get a KMU push key.
 *
 * @param[in] slot_number         Key slot number.
 * @param[out] attributes         Key attributes.
 * @param[out] key_buffer         Buffer representing the built-in key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the built-in key in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_DOES_NOT_EXIST   The key does not exist.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t kmu_push_get_builtin_key(psa_drv_slot_number_t slot_number,
				      psa_key_attributes_t *attributes, uint8_t *key_buffer,
				      size_t key_buffer_size, size_t *key_buffer_length);

/** @brief Function to destroying a KMU push key
 *
 * @param[in] attributes Attributes to the key to destroy.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    Destroying unsupported type (revokable or read-only)
 * @retval PSA_ERROR_INVALID_ARGUMENT One or more input parameters are wrong.
 * @retval PSA_ERROR_NOT_PERMITTED    The KMU slot(s) are invalid (e.g. revoked or other format).
 * @retval PSA_ERROR_DATA_INVALID     KMU slot(s) contain invalid or partial data.
 * @retval PSA_ERROR_BAD_STATE        The KMU metadata retrieval failed with unknown error.
 */
psa_status_t kmu_push_destroy_key(const psa_key_attributes_t *attributes);

#endif /* KMU_PUSH_H */
