/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_SW_COMMON_H
#define CRACEN_SW_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <psa/crypto.h>
#include <sxsymcrypt/keyref.h>

/**
 * @brief Encrypt a single AES block using ECB mode.
 *
 * This function is designed for CRACEN software workarounds that need
 * AES-ECB encryption functionality.
 *
 * @param blkciph The block cipher struct.
 * @param key The AES key reference.
 * @param input Pointer to the input block.
 * @param input_length Length of the input block.
 * @param output Pointer to a 16-byte output buffer.
 * @param output_size Size of the output buffer.
 * @param output_length Pointer where to store the actual output length.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_sw_aes_ecb_encrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length);

/** @brief Decrypt a single AES block using ECB mode.
 *
 * This function is designed for CRACEN software workarounds that need
 * AES-ECB decryption functionality.
 *
 * @param blkciph The block cipher struct.
 * @param key The AES key reference.
 * @param input Pointer to the input block.
 * @param input_length Length of the input block.
 * @param output Pointer to a 16-byte output buffer.
 * @param output_size Size of the output buffer.
 * @param output_length Pointer where to store the actual output length.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_sw_aes_ecb_decrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length);

/** @brief Perform a single AES block encryption operation.
 *
 * This function is designed for use as an AES primitive in CRACEN software workarounds that require
 * it.
 *
 * @param blkciph The block cipher struct.
 * @param key The AES key reference.
 * @param input Pointer to the input block.
 * @param output Pointer to a 16-byte output buffer.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_sw_aes_primitive(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				     const uint8_t *input, uint8_t *output);

/** @brief Increment counter value stored as a big endian buffer.
 *
 * This function is used by software workarounds for CRACEN peripheral.
 *
 * @param[in,out] ctr_buf Counter.
 * @param[in] ctr_buf_size Counter buffer size.
 * @param[in] start_pos Counter starting position index.
 *
 * @retval PSA_SUCCESS		      The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT The counter overflowed.
 */
psa_status_t cracen_sw_increment_counter_be(uint8_t *ctr_buf, size_t ctr_buf_size,
					    size_t start_pos);

/** @brief Encode value as big-endian, right-aligned in buffer.
 *
 * This function is used by software workarounds for CRACEN peripheral.
 *
 * @param[out] buffer Output buffer.
 * @param[in] buffer_size Output buffer size.
 * @param[in] value Value to encode.
 * @param[in] value_size A size of the value.
 */
void cracen_sw_encode_value_be(uint8_t *buffer, size_t buffer_size, size_t value,
			       size_t value_size);

#endif /* CRACEN_SW_COMMON_H */
