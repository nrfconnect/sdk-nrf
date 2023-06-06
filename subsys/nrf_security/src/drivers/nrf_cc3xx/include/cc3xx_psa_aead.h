/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef CC3XX_PSA_AEAD_H
#define CC3XX_PSA_AEAD_H

/** \file cc3xx_psa_aead.h
 *
 * This file contains the declaration of the entry points associated to the
 * aead capability (single-part and multipart) as described by the PSA
 * Cryptoprocessor Driver interface specification
 *
 */

#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Encrypt and authenticate with an AEAD algorithm in one-shot
 *
 * \param[in]  attributes             Attributes of the key to use
 * \param[in]  key_buffer             Key material buffer
 * \param[in]  key_buffer_size        Size in bytes of the key
 * \param[in]  alg                    Algorithm to use
 * \param[in]  nonce                  Nonce buffer
 * \param[in]  nonce_length           Size in bytes of the nonce
 * \param[in]  additional_data        Data to authenticate buffer
 * \param[in]  additional_data_length Size in bytes of additional_data
 * \param[in]  plaintext              Data to encrypt buffer
 * \param[in]  plaintext_length       Size in bytes of the data to encrypt
 * \param[out] ciphertext             Buffer to hold the encrypted data
 * \param[in]  ciphertext_size        Size in bytes of the ciphertext buffer
 * \param[out] ciphertext_length      Size in bytes of the encrypted data
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t
cc3xx_aead_encrypt(const psa_key_attributes_t *attributes,
                   const uint8_t *key_buffer, size_t key_buffer_size,
                   psa_algorithm_t alg, const uint8_t *nonce,
                   size_t nonce_length, const uint8_t *additional_data,
                   size_t additional_data_length, const uint8_t *plaintext,
                   size_t plaintext_length, uint8_t *ciphertext,
                   size_t ciphertext_size, size_t *ciphertext_length);

/*!
 * \brief Decrypt and verify tag with an AEAD algorithm in one-shot
 *
 * \param[in]  attributes             Attributes of the key to use
 * \param[in]  key_buffer             Key material buffer
 * \param[in]  key_buffer_size        Size in bytes of the key
 * \param[in]  alg                    Algorithm to use
 * \param[in]  nonce                  Nonce buffer
 * \param[in]  nonce_length           Size in bytes of the nonce
 * \param[in]  additional_data        Data to authenticate buffer
 * \param[in]  additional_data_length Size in bytes of additional_data
 * \param[in]  ciphertext             Data to decrypt buffer
 * \param[in]  ciphertext_length      Size in bytes of the data to decrypt
 * \param[out] plaintext              Buffer to hold the decrypted data
 * \param[in]  plaintext_size         Size in bytes of the plaintext buffer
 * \param[out] plaintext_length       Size in bytes of the decrypted data
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t
cc3xx_aead_decrypt(const psa_key_attributes_t *attributes,
                   const uint8_t *key_buffer, size_t key_buffer_size,
                   psa_algorithm_t alg, const uint8_t *nonce,
                   size_t nonce_length, const uint8_t *additional_data,
                   size_t additional_data_length, const uint8_t *ciphertext,
                   size_t ciphertext_length, uint8_t *plaintext,
                   size_t plaintext_size, size_t *plaintext_length);

/*!
 * \brief Set the key for a multipart authenticated encryption operation.
 *
 * \param[in] operation       The operation object to set up.
 * \param[in] attributes      The attributes of the key to be set.
 * \param[in] key_buffer      The buffer containing the key material.
 * \param[in] key_buffer_size Size of \p key_buffer in bytes.
 * \param[in] alg             The AEAD algorithm
 *                            (#PSA_ALG_IS_AEAD(\p alg) must be true).
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_BAD_STATE
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t cc3xx_aead_encrypt_setup(
        cc3xx_aead_operation_t *operation,
        const psa_key_attributes_t *attributes,
        const uint8_t *key_buffer, size_t key_buffer_size,
        psa_algorithm_t alg);

/*!
 * \brief Set the key for a multipart authenticated decryption operation
 *
 * \param[in] operation       The operation object to set up.
 * \param[in] attributes      The attributes of the key to be set.
 * \param[in] key_buffer      The buffer containing the key material.
 * \param[in] key_buffer_size Size of \p key_buffer in bytes.
 * \param[in] alg             The AEAD algorithm
 *                            (#PSA_ALG_IS_AEAD(\p alg) must be true).
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_BAD_STATE
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t cc3xx_aead_decrypt_setup(
        cc3xx_aead_operation_t *operation,
        const psa_key_attributes_t *attributes,
        const uint8_t *key_buffer, size_t key_buffer_size,
        psa_algorithm_t alg);

/*!
 * \brief Set the nonce for an authenticated encryption or decryption operation.
 *
 * \param[in] operation    Active AEAD operation.
 * \param[in] nonce        Buffer containing the nonce to use.
 * \param[in] nonce_length Size of the nonce in bytes.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_GENERIC_ERROR
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_DATA_INVALID
 */
psa_status_t cc3xx_aead_set_nonce(
        cc3xx_aead_operation_t *operation,
        const uint8_t *nonce,
        size_t nonce_length);

/*!
 * \brief Declare the lengths of the message and additional data for AEAD
 *
 * \param[in] operation        Active AEAD operation.
 * \param[in] ad_length        Size of the additional data in bytes.
 * \param[in] plaintext_length Size of the plaintext to encrypt in bytes.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t cc3xx_aead_set_lengths(
        cc3xx_aead_operation_t *operation,
        size_t ad_length,
        size_t plaintext_length);

/*!
 * \brief Pass additional data to an active AEAD operation
 *
 * \param[in] operation    Active AEAD operation.
 * \param[in] input        Buffer containing the additional data.
 * \param[in] input_length Size of the input buffer in bytes.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_DATA_INVALID
 */
psa_status_t cc3xx_aead_update_ad(
        cc3xx_aead_operation_t *operation,
        const uint8_t *input,
        size_t input_length);

/*!
 * \brief Encrypt or decrypt a message fragment in an active AEAD operation.
 *
 * \param[in]  operation     Active AEAD operation.
 * \param[in]  input         Buffer containing the message fragment.
 * \param[in]  input_length  Size of the input buffer in bytes.
 * \param[out] output        Buffer where the output is to be written.
 * \param[in]  output_size   Size of the output buffer in bytes.
 * \param[out] output_length The number of bytes that make up the output.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_DATA_INVALID
 */
psa_status_t cc3xx_aead_update(
        cc3xx_aead_operation_t *operation,
        const uint8_t *input,
        size_t input_length,
        uint8_t *output,
        size_t output_size,
        size_t *output_length);

/*!
 * \brief Finish encrypting a message in an AEAD operation.
 *
 * \param[in] operation          Active AEAD operation.
 * \param[out] ciphertext        Buffer containing the ciphertext.
 * \param[in] ciphertext_size    Size of the ciphertext buffer in bytes.
 * \param[out] ciphertext_length The number of bytes that make up the ciphertext
 * \param[out] tag               Buffer where the tag is to be written.
 * \param[in] tag_size           Size of the tag buffer in bytes.
 * \param[out] tag_length        The number of bytes that make up the tag.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t cc3xx_aead_finish(
        cc3xx_aead_operation_t *operation,
        uint8_t *ciphertext,
        size_t ciphertext_size,
        size_t *ciphertext_length,
        uint8_t *tag,
        size_t tag_size,
        size_t *tag_length);

/*!
 * \brief Finish decrypting a message in an AEAD operation.
 *
 * \param[in] operation         Active AEAD operation.
 * \param[out] plaintext        Buffer containing the plaintext.
 * \param[in] plaintext_size    Size of the plaintext buffer in bytes.
 * \param[out] plaintext_length The number of bytes that make up the plaintext
 * \param[in] tag               Buffer containing the tag
 * \param[in] tag_length        Size of the tag buffer in bytes
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t cc3xx_aead_verify(
        cc3xx_aead_operation_t *operation,
        uint8_t *plaintext,
        size_t plaintext_size,
        size_t *plaintext_length,
        const uint8_t *tag,
        size_t tag_length);
/*!
 * \brief Abort an AEAD operation
 *
 * \param[out] operation Initialized AEAD operation.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t cc3xx_aead_abort(cc3xx_aead_operation_t *operation);

#ifdef __cplusplus
}
#endif
#endif /* CC3XX_PSA_AEAD_H */
