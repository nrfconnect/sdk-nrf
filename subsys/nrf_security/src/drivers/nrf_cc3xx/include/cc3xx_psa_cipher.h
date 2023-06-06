/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef CC3XX_PSA_CIPHER_H
#define CC3XX_PSA_CIPHER_H

/** \file cc3xx_psa_cipher.h
 *
 * This file contains the declaration of the entry points associated to the
 * cipher capability (single-part and multipart) as described by the PSA
 * Cryptoprocessor Driver interface specification
 *
 */

#include "psa/crypto.h"
#include "cc3xx_crypto_primitives.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Setup a cipher encryption object by binding the key to the
 *        context of a multipart cipher operation
 *
 * \param[in] operation  Pointer to the operation object
 * \param[in] attributes Attributes for the key to be set
 * \param[in] key        Buffer holding key material
 * \param[in] key_length Size in bytes of the key
 * \param[in] alg        Algorithm to setup for which PSA_ALG_IS_CIPHER(alg)
 *                       must be true
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_BAD_STATE
 *
 */
psa_status_t cc3xx_cipher_encrypt_setup(
        cc3xx_cipher_operation_t *operation,
        const psa_key_attributes_t *attributes,
        const uint8_t *key, size_t key_length,
        psa_algorithm_t alg);
/*!
 * \brief Setup a cipher decryption object by binding the key to the
 *        context of a multipart cipher operation
 *
 * \param[in] operation  Pointer to the operation object
 * \param[in] attributes Attributes for the key to be set
 * \param[in] key        Buffer holding key material
 * \param[in] key_length Size in bytes of the key
 * \param[in] alg        Algorithm to setup for which PSA_ALG_IS_CIPHER(alg)
 *                       must be true
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_BAD_STATE
 *
 */
psa_status_t cc3xx_cipher_decrypt_setup(
        cc3xx_cipher_operation_t *operation,
        const psa_key_attributes_t *attributes,
        const uint8_t *key, size_t key_length,
        psa_algorithm_t alg);
/*!
 * \brief Setup an IV on the operation object
 *
 * \param[in] operation Pointer to the operation object
 * \param[in] iv        Buffer holding the IV to setup
 * \param[in] iv_length Size in bytes of the IV
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_cipher_set_iv(
        cc3xx_cipher_operation_t *operation,
        const uint8_t *iv, size_t iv_length);
/*!
 * \brief Update the ciphering operation with new input to produce
 *        new output
 *
 * \param[in]  operation     Pointer to the operation object
 * \param[in]  input         Buffer holding the IV to setup
 * \param[in]  input_length  Size in bytes of the IV
 * \param[out] output        Buffer containing the output
 * \param[in]  output_size   Size in bytes of the output buffer
 * \param[out] output_length Size in bytes of the actual output
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_cipher_update(
        cc3xx_cipher_operation_t *operation,
        const uint8_t *input, size_t input_length,
        uint8_t *output, size_t output_size, size_t *output_length);
/*!
 * \brief Finalize the multipart cipher operation by encrypting any
 *        buffered input from the previous call to update()
 *
 * \param[in]  operation     Pointer to the operation object
 * \param[out] output        Buffer containing the
 * \param[in]  output_size   Size in bytes of the output buffer
 * \param[out] output_length Size in bytes of the encrypted output
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_cipher_finish(
        cc3xx_cipher_operation_t *operation,
        uint8_t *output, size_t output_size, size_t *output_length);
/*!
 * \brief Abort a multipart cipher operation
 *
 * \param[out] operation Pointer to the operation object
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_cipher_abort(
        cc3xx_cipher_operation_t *operation);
/*!
 * \brief Perform a one-shot encryption operation
 *
 * \param[in]  attributes      Key attributes
 * \param[in]  key_buffer      Buffer holding key material
 * \param[in]  key_buffer_size Size in bytes of the key
 * \param[in]  alg             Algorithm to be used for which
 *                             PSA_ALG_IS_CIPHER(alg) is true
 * \param[in]  iv              Buffer holding the initialization vector
 * \param[in]  iv_length       Size in bytes of the initialization vector
 * \param[in]  input           Buffer holding the plaintext
 * \param[in]  input_length    Size in bytes of the plaintext
 * \param[out] output          Buffer holding the ciphertext
 * \param[in]  output_size     Size in bytes of the output buffer
 * \param[out] output_length   Size in bytes of the encrypted data
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_cipher_encrypt(
        const psa_key_attributes_t *attributes,
        const uint8_t *key_buffer,
        size_t key_buffer_size,
        psa_algorithm_t alg,
        const uint8_t *iv,
        size_t iv_length,
        const uint8_t *input,
        size_t input_length,
        uint8_t *output,
        size_t output_size,
        size_t *output_length);
/*!
 * \brief Perform a one-shot decryption operation
 *
 * \param[in]  attributes      Key attributes
 * \param[in]  key_buffer      Buffer holding key material
 * \param[in]  key_buffer_size Size in bytes of the key
 * \param[in]  alg             Algorithm to be used for which
 *                             PSA_ALG_IS_CIPHER(alg) is true
 * \param[in]  input           Buffer holding the ciphertext
 * \param[in]  input_length    Size in bytes of the ciphertext
 * \param[out] output          Buffer holding the ciphertext
 * \param[in]  output_size     Size in bytes of the output buffer
 * \param[out] output_length   Size in bytes of the decrypted data
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_cipher_decrypt(
        const psa_key_attributes_t *attributes,
        const uint8_t *key_buffer,
        size_t key_buffer_size,
        psa_algorithm_t alg,
        const uint8_t *input,
        size_t input_length,
        uint8_t *output,
        size_t output_size,
        size_t *output_length);

#ifdef __cplusplus
}
#endif
#endif /* CC3XX_PSA_CIPHER_H */
