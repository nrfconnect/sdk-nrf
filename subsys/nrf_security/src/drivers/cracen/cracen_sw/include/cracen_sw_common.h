/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_sw_common
 * @{
 * @brief Shared primitives for CRACEN software implementation.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_SW_COMMON_H
#define CRACEN_SW_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <psa/crypto.h>
#include <sxsymcrypt/keyref.h>

/** Number of AES blocks the stream helpers in this file run per CRACEN job. Each helper
 * call needs 16 bytes of stack per block, for the keystream or the CBC output.
 */
#define CRACEN_SW_AES_BATCH_BLOCKS 8

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
 * it. The caller must reserve CRACEN first, with
 * sx_hw_reserve(&blkciph->dma, SX_HW_RESERVE_CM_ENABLED), and can then run any number of blocks
 * before releasing it with sx_hw_release().
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

/** @brief AES-CTR encryption or decryption of a byte stream.
 *
 * Uses up the buffered keystream block first, then runs whole blocks with up to
 * CRACEN_SW_AES_BATCH_BLOCKS blocks per CRACEN job, and buffers the keystream of
 * a final partial block for the next call. The counter wraps modulo the size of
 * its field. CRACEN must be reserved by the caller, as for
 * cracen_sw_aes_primitive().
 *
 * @param blkciph Block cipher struct.
 * @param key The AES key reference.
 * @param counter Counter block for the next keystream block, advanced as used.
 * @param counter_start_pos Index of the first counter byte in @p counter.
 * @param keystream Buffered keystream block (16 bytes).
 * @param keystream_offset Bytes of @p keystream already used, 16 if none is left.
 * @param input Input.
 * @param output Output, may be the same as @p input.
 * @param length Number of bytes.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_sw_ctr_xor(struct sxblkcipher *blkciph, const struct sxkeyref *key,
			       uint8_t *counter, size_t counter_start_pos, uint8_t *keystream,
			       size_t *keystream_offset, const uint8_t *input, uint8_t *output,
			       size_t length);

/** @brief CBC-MAC over a byte stream.
 *
 * Complete blocks are absorbed into @p mac_state with up to
 * CRACEN_SW_AES_BATCH_BLOCKS blocks per CRACEN job, as AES-CBC encryption with
 * the MAC state as IV. A trailing partial block is kept in @p partial. CRACEN
 * must be reserved by the caller, as for cracen_sw_aes_primitive().
 *
 * @param blkciph Block cipher struct.
 * @param key The AES key reference.
 * @param mac_state CBC-MAC state (16 bytes), updated in place.
 * @param partial Buffered partial block (16 bytes).
 * @param partial_len Bytes in @p partial.
 * @param input Input.
 * @param length Number of bytes.
 * @param keep_last Keep the last block buffered even when it is full, until more
 *                  input follows (for CMAC, which treats the last block apart).
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_sw_cbc_mac_update(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				      uint8_t *mac_state, uint8_t *partial, size_t *partial_len,
				      const uint8_t *input, size_t length, bool keep_last);

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

/** @} */

#endif /* CRACEN_SW_COMMON_H */
