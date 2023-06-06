/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef OBERON_AEAD_H
#define OBERON_AEAD_H

#include <psa/crypto_driver_common.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    struct {
#if defined(PSA_NEED_OBERON_AES_GCM)
        uint32_t a[77];
#elif defined(PSA_NEED_OBERON_AES_CCM)
        uint32_t a[73];
#else /* PSA_NEED_OBERON_CHACHA20_POLY1305 */
        uint32_t a[51];
#endif
        size_t s[2];
    } ctx;
    size_t ad_length;
    size_t pt_length;
    psa_algorithm_t alg;
    uint8_t decrypt;
    uint8_t length_set;
    uint8_t tag_length;
} oberon_aead_operation_t;


psa_status_t oberon_aead_encrypt_setup(
    oberon_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg);

psa_status_t oberon_aead_decrypt_setup(
    oberon_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg);

psa_status_t oberon_aead_set_lengths(
    oberon_aead_operation_t *operation,
    size_t ad_length,
    size_t plaintext_length);

psa_status_t oberon_aead_set_nonce(
    oberon_aead_operation_t *operation,
    const uint8_t *nonce, size_t nonce_length);

psa_status_t oberon_aead_update_ad(
    oberon_aead_operation_t *operation,
    const uint8_t *input, size_t input_length);

psa_status_t oberon_aead_update(
    oberon_aead_operation_t *operation,
    const uint8_t *input, size_t input_length,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_aead_finish(
    oberon_aead_operation_t *operation,
    uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_length,
    uint8_t *tag, size_t tag_size, size_t *tag_length);

psa_status_t oberon_aead_verify(
    oberon_aead_operation_t *operation,
    uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_length,
    const uint8_t *tag, size_t tag_length);

psa_status_t oberon_aead_abort(
    oberon_aead_operation_t *operation);


psa_status_t oberon_aead_encrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *plaintext, size_t plaintext_length,
    uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_length);

psa_status_t oberon_aead_decrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *nonce, size_t nonce_length,
    const uint8_t *additional_data, size_t additional_data_length,
    const uint8_t *ciphertext, size_t ciphertext_length,
    uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_length);


#ifdef __cplusplus
}
#endif

#endif
