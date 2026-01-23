/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_KEY_MANAGEMENT_H
#define CRACEN_PSA_KEY_MANAGEMENT_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Export a public key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[out] data           Buffer to store the public key.
 * @param[in] data_size       Size of the data buffer in bytes.
 * @param[out] data_length    Length of the exported public key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The data buffer is too small.
 */
psa_status_t cracen_export_public_key(const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length);

/** @brief Import a key.
 *
 * @param[in] attributes          Key attributes.
 * @param[in] data                Key data to import.
 * @param[in] data_length         Length of the key data in bytes.
 * @param[out] key_buffer         Buffer to represent the stored key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the imported key in bytes.
 * @param[out] key_bits           Size of the key in bits.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key data is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			       size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			       size_t *key_buffer_length, size_t *key_bits);

/** @brief Generate a key.
 *
 * @param[in] attributes          Key attributes.
 * @param[out] key_buffer         Buffer to represent the generated key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the generated key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED  The key type or size is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				 size_t key_buffer_size, size_t *key_buffer_length);

/** @brief Get a built-in key.
 *
 * @param[in] slot_number         Key slot number.
 * @param[out] attributes         Key attributes.
 * @param[out] key_buffer         Buffer representing the built-in key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the built-in key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_DOES_NOT_EXIST The key does not exist.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_get_builtin_key(psa_drv_slot_number_t slot_number,
				    psa_key_attributes_t *attributes, uint8_t *key_buffer,
				    size_t key_buffer_size, size_t *key_buffer_length);

/** @brief Export a key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[out] data           Buffer to store the exported key.
 * @param[in] data_size       Size of the data buffer in bytes.
 * @param[out] data_length    Length of the exported key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The data buffer is too small.
 */
psa_status_t cracen_export_key(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size, uint8_t *data, size_t data_size,
			       size_t *data_length);

/** @brief Copy a key.
 *
 * @param[in,out] attributes             Key attributes for the target key.
 * @param[in] source_key                 Source key material.
 * @param[in] source_key_length          Length of the source key in bytes.
 * @param[out] target_key_buffer         Buffer to store the copied key.
 * @param[in] target_key_buffer_size     Size of the target key buffer in bytes.
 * @param[out] target_key_buffer_length  Length of the target key buffer for the copied key
 *                                       in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The target key buffer is too small.
 */
psa_status_t cracen_copy_key(psa_key_attributes_t *attributes, const uint8_t *source_key,
			     size_t source_key_length, uint8_t *target_key_buffer,
			     size_t target_key_buffer_size, size_t *target_key_buffer_length);

/** @brief Destroy a key.
 *
 * @param[in] attributes Key attributes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 */
psa_status_t cracen_destroy_key(const psa_key_attributes_t *attributes);

/** @brief Derive a key from input material.
 *
 * @param[in] attributes      Key attributes for the derived key.
 * @param[in] input           Input material.
 * @param[in] input_length    Length of the input material in bytes.
 * @param[out] key            Buffer to store the derived key.
 * @param[in] key_size        Size of the key buffer in bytes.
 * @param[out] key_length     Length of the derived key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED  The key derivation is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_derive_key(const psa_key_attributes_t *attributes, const uint8_t *input,
			       size_t input_length, uint8_t *key, size_t key_size,
			       size_t *key_length);

/** @brief Get the key slot for a given key ID.
 *
 * @param[in] key_id       Key ID.
 * @param[out] lifetime    Key lifetime.
 * @param[out] slot_number Key slot number.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST The key does not exist.
 */
psa_status_t cracen_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				 psa_drv_slot_number_t *slot_number);

#endif /* CRACEN_PSA_KEY_MANAGEMENT_H */
