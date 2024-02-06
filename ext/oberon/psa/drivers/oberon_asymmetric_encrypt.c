/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file implements functions from the Arm PSA Crypto Driver API.

#include "psa/crypto.h"
#include "oberon_asymmetric_encrypt.h"

#ifdef PSA_NEED_OBERON_RSA_ANY_CRYPT
#include "oberon_rsa.h"
#endif


psa_status_t oberon_asymmetric_encrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    const uint8_t *salt, size_t salt_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_RSA_ANY_CRYPT
    if (PSA_KEY_TYPE_IS_RSA(type)) {
        return oberon_rsa_encrypt(
            attributes, key, key_length,
            alg,
            input, input_length,
            salt, salt_length,
            output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_RSA_ANY_CRYPT */

    {
        (void)key;
        (void)key_length;
        (void)alg;
        (void)input;
        (void)input_length;
        (void)salt;
        (void)salt_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        (void)type;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_asymmetric_decrypt(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    const uint8_t *salt, size_t salt_length,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_RSA_ANY_CRYPT
    if (PSA_KEY_TYPE_IS_RSA(type)) {
        return oberon_rsa_decrypt(
            attributes, key, key_length,
            alg,
            input, input_length,
            salt, salt_length,
            output, output_size, output_length);
    } else
#endif /* PSA_NEED_OBERON_RSA_ANY_CRYPT */

    {
        (void)key;
        (void)key_length;
        (void)alg;
        (void)input;
        (void)input_length;
        (void)salt;
        (void)salt_length;
        (void)output;
        (void)output_size;
        (void)output_length;
        (void)type;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

