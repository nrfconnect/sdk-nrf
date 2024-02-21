/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file implements functions from the Arm PSA Crypto Driver API.

#include "psa/crypto.h"
#include "oberon_key_management.h"
#include "oberon_ec_keys.h"
#include "oberon_rsa.h"
#include "oberon_spake2p.h"
#include "oberon_srp.h"


psa_status_t oberon_export_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    uint8_t *data, size_t data_size, size_t *data_length)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT
    if (PSA_KEY_TYPE_IS_ECC(type)) {
        return oberon_export_ec_public_key(
            attributes, key, key_length,
            data, data_size, data_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT */

#ifdef PSA_NEED_OBERON_KEY_TYPE_RSA_KEY_PAIR_EXPORT
    if (PSA_KEY_TYPE_IS_RSA(type)) {
        return oberon_export_rsa_public_key(
            attributes, key, key_length,
            data, data_size, data_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_RSA_KEY_PAIR_EXPORT */

#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT
    if (PSA_KEY_TYPE_IS_SPAKE2P(type)) {
        return oberon_export_spake2p_public_key(
            attributes, key, key_length,
            data, data_size, data_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT */

#ifdef PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_EXPORT
    if (PSA_KEY_TYPE_IS_SRP(type)) {
        return oberon_export_srp_public_key(
            attributes, key, key_length,
            data, data_size, data_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_EXPORT */

    {
        (void)key;
        (void)key_length;
        (void)data;
        (void)data_size;
        (void)data_length;
        (void)type;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_import_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key, size_t key_size, size_t *key_length,
    size_t *key_bits)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT
    if (PSA_KEY_TYPE_IS_ECC(type)) {
        return oberon_import_ec_key(
            attributes, data, data_length,
            key, key_size, key_length, key_bits);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT */

#ifdef PSA_NEED_OBERON_KEY_TYPE_RSA_KEY_PAIR_IMPORT
    if (PSA_KEY_TYPE_IS_RSA(type)) {
        return oberon_import_rsa_key(
            attributes, data, data_length,
            key, key_size, key_length, key_bits);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_RSA_KEY_PAIR_IMPORT */

#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT
    if (PSA_KEY_TYPE_IS_SPAKE2P(type)) {
        return oberon_import_spake2p_key(
            attributes, data, data_length,
            key, key_size, key_length, key_bits);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT */

#ifdef PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_IMPORT
    if (PSA_KEY_TYPE_IS_SRP(type)) {
        return oberon_import_srp_key(
            attributes, data, data_length,
            key, key_size, key_length, key_bits);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_SRP_6_KEY_PAIR_IMPORT */

    {
        (void)data;
        (void)data_length;
        (void)key;
        (void)key_size;
        (void)key_length;
        (void)key_bits;
        (void)type;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_generate_key(
    const psa_key_attributes_t *attributes,
    uint8_t *key, size_t key_size, size_t *key_length)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE
    if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(type)) {
        return oberon_generate_ec_key(
            attributes,
            key, key_size, key_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE */

    {
        (void)key;
        (void)key_size;
        (void)key_length;
        (void)type;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_derive_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *input, size_t input_length,
    uint8_t *key, size_t key_size, size_t *key_length)
{
    psa_key_type_t type = psa_get_key_type(attributes);

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE
    if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(type)) {
        return oberon_derive_ec_key(
            attributes, input, input_length,
            key, key_size, key_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE */

#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE
    if (PSA_KEY_TYPE_IS_SPAKE2P_KEY_PAIR(type)) {
        return oberon_derive_spake2p_key(
            attributes, input, input_length,
            key, key_size, key_length);
    } else
#endif /* PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE */

    {
        (void)input;
        (void)input_length;
        (void)key;
        (void)key_size;
        (void)key_length;
        (void)type;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}
