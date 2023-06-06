/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef CC3XX_PSA_ASYMMETRIC_SIGNATURE_H
#define CC3XX_PSA_ASYMMETRIC_SIGNATURE_H

/** \file cc3xx_psa_asymmetric_signature.h
 *
 * This file contains the declaration of the entry points associated to the
 * asymmetric signature capability as described by the PSA Cryptoprocessor
 * Driver interface specification
 *
 */

#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Sign a message
 *
 * \param[in]  attributes       Attributes of the key to use
 * \param[in]  key_buffer       Key material buffer
 * \param[in]  key_buffer_size  Size in bytes of the key
 * \param[in]  alg              Algorithm to use
 * \param[in]  input            Data to sign buffer
 * \param[in]  input_length     Size in bytes of the data to sign
 * \param[out] signature        Buffer to hold the signature data
 * \param[in]  signature_size   Size in bytes of the signature buffer
 * \param[out] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_sign_message(const psa_key_attributes_t *attributes,
                                const uint8_t *key, size_t key_length,
                                psa_algorithm_t alg, const uint8_t *input,
                                size_t input_length, uint8_t *signature,
                                size_t signature_size,
                                size_t *signature_length);
/*!
 * \brief Verify a message signature
 *
 * \param[in] attributes       Attributes of the key to use
 * \param[in] key_buffer       Key material buffer
 * \param[in] key_buffer_size  Size in bytes of the key
 * \param[in] alg              Algorithm to use
 * \param[in] input            Data to sign buffer
 * \param[in] input_length     Size in bytes of the data to sign
 * \param[in] signature        Signature to verify
 * \param[in] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_verify_message(const psa_key_attributes_t *attributes,
                                  const uint8_t *key, size_t key_length,
                                  psa_algorithm_t alg, const uint8_t *input,
                                  size_t input_length, const uint8_t *signature,
                                  size_t signature_length);
/*!
 * \brief Sign a precomputed hash of a message
 *
 * \param[in]  attributes       Attributes of the key to use
 * \param[in]  key_buffer       Key material buffer
 * \param[in]  key_buffer_size  Size in bytes of the key
 * \param[in]  alg              Algorithm to use
 * \param[in]  input            Hash to sign buffer
 * \param[in]  input_length     Size in bytes of the data to sign
 * \param[out] signature        Buffer to hold the signature data
 * \param[in]  signature_size   Size in bytes of the signature buffer
 * \param[out] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_sign_hash(const psa_key_attributes_t *attributes,
                             const uint8_t *key, size_t key_length,
                             psa_algorithm_t alg, const uint8_t *input,
                             size_t input_length, uint8_t *signature,
                             size_t signature_size, size_t *signature_length);
/*!
 * \brief Verify a message signature on a hash
 *
 * \param[in] attributes       Attributes of the key to use
 * \param[in] key_buffer       Key material buffer
 * \param[in] key_buffer_size  Size in bytes of the key
 * \param[in] alg              Algorithm to use
 * \param[in] input            Hash to sign buffer
 * \param[in] input_length     Size in bytes of the data to sign
 * \param[in] signature        Signature to verify
 * \param[in] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t cc3xx_verify_hash(const psa_key_attributes_t *attributes,
                               const uint8_t *key, size_t key_length,
                               psa_algorithm_t alg, const uint8_t *hash,
                               size_t hash_length, const uint8_t *signature,
                               size_t signature_length);
#ifdef __cplusplus
}
#endif
#endif /* CC3XX_PSA_ASYMMETRIC_SIGNATURE_H */
