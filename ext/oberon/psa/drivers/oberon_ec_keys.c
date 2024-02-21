/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file implements functions from the Arm PSA Crypto Driver API.

#include <string.h>

#include "psa/crypto.h"
#include "oberon_ec_keys.h"
#include "oberon_helpers.h"

#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_224) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_224) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_224) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_224)
#include "ocrypto_ecdh_p224.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_256) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_256) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_256) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_256)
#include "ocrypto_ecdh_p256.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_384) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_384) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_384) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_384)
#include "ocrypto_ecdh_p384.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_521) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_521)
#include "ocrypto_ecdh_p521.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255)
#include "ocrypto_curve25519.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448)
#include "ocrypto_curve448.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_255)
#include "ocrypto_ed25519.h"
#endif
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_448)
#include "ocrypto_ed448.h"
#endif


psa_status_t oberon_export_ec_public_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    uint8_t *data, size_t data_size, size_t *data_length)
{
    int res;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);

    if (PSA_KEY_TYPE_IS_PUBLIC_KEY(type)) {
        if (key_length > data_size) return PSA_ERROR_BUFFER_TOO_SMALL;
        memcpy(data, key, key_length);
        *data_length = key_length;
        return PSA_SUCCESS;
    }

    switch (type) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (data_size < key_length * 2 + 1) return PSA_ERROR_BUFFER_TOO_SMALL;
        *data_length = key_length * 2 + 1;
        data[0] = 0x04;
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_224
        case 224:
            res = ocrypto_ecdh_p224_public_key(&data[1], key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_256
        case 256:
            res = ocrypto_ecdh_p256_public_key(&data[1], key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_384
        case 384:
            res = ocrypto_ecdh_p384_public_key(&data[1], key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_521
        case 521:
            res = ocrypto_ecdh_p521_public_key(&data[1], key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_521 */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        if (res) return PSA_ERROR_INVALID_ARGUMENT;
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
        if (data_size < key_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *data_length = key_length;
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_255
        case 255:
            ocrypto_curve25519_scalarmult_base(data, key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_255 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_448
        case 448:
            ocrypto_curve448_scalarmult_base(data, key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY_448 */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_MONTGOMERY */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        if (data_size < key_length) return PSA_ERROR_BUFFER_TOO_SMALL;
        *data_length = key_length;
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_255
        case 255:
            ocrypto_ed25519_public_key(data, key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_255 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_448
        case 448:
            ocrypto_ed448_public_key(data, key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS_448 */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_TWISTED_EDWARDS */
    default:
        (void)res;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY)
static void oberon_set_forced_bits(uint8_t *key, size_t bits)
{
    switch (bits) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_255)
    case 255:
        key[0]  = (uint8_t)(key[0] & 0xF8);
        key[31] = (uint8_t)((key[31] & 0x7F) | 0x40);
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255 ||
        PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255 ||
        PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_448)
    case 448:
        key[0]  = (uint8_t)(key[0] & 0xFC);
        key[55] = (uint8_t)(key[55] | 0x80);
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448 ||
        PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448 ||
        PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_448 */
    }
}
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY ||
        PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY ||
        PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY */

psa_status_t oberon_import_ec_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key, size_t key_size, size_t *key_length,
    size_t *key_bits)
{
    int res;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (bits == 0) {
            bits = PSA_BYTES_TO_BITS(data_length);
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521
            if (bits == 528) bits = 521;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521 */
        }
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_224
        case 224:
            if (data_length != 28) return PSA_ERROR_INVALID_ARGUMENT;
            if (!oberon_ct_compare_zero(data, 28)) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p224_secret_key_check(data);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_256
        case 256:
            if (data_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
            if (!oberon_ct_compare_zero(data, 32)) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p256_secret_key_check(data);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_384
        case 384:
            if (data_length != 48) return PSA_ERROR_INVALID_ARGUMENT;
            if (!oberon_ct_compare_zero(data, 48)) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p384_secret_key_check(data);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521
        case 521:
            if (data_length != 66) return PSA_ERROR_INVALID_ARGUMENT;
            if (!oberon_ct_compare_zero(data, 66)) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p521_secret_key_check(data);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP_R1_521 */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // out of range
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_SECP */

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP
    case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1):
        if (bits == 0) {
            if ((data_length & 1) == 0) return PSA_ERROR_INVALID_ARGUMENT;
            bits = PSA_BYTES_TO_BITS(data_length >> 1);
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521
            if (bits == 528) bits = 521;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521 */
        }
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_224
        case 224:
            if (data_length != 57 || data[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p224_public_key_check(&data[1]);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_256
        case 256:
            if (data_length != 65 || data[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p256_public_key_check(&data[1]);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_384
        case 384:
            if (data_length != 97 || data[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p384_public_key_check(&data[1]);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521
        case 521:
            if (data_length != 133 || data[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
            res = ocrypto_ecdh_p521_public_key_check(&data[1]);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP_R1_521 */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // point not on curve
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_SECP */

#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY)
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY
    case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY):
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY */
        if (bits == 0) {
            switch (data_length) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255)
            case 32: bits = 255; break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448)
            case 56: bits = 448; break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448 */
            default: return PSA_ERROR_NOT_SUPPORTED;
            }
        }
        if (data_length != PSA_BITS_TO_BYTES(bits)) return PSA_ERROR_INVALID_ARGUMENT;
        switch (bits) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255)
        case 255: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_255 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448)
        case 448: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY_448 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY_448 */
        default: return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_MONTGOMERY */

#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS)
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS
    case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS):
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS */
        if (bits == 0) {
            switch (data_length) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255)
            case 32: bits = 255; break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448)
            case 57: bits = 448; break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448 */
            default: return PSA_ERROR_NOT_SUPPORTED;
            }
        }
        if (data_length != PSA_BITS_TO_BYTES(bits + 1)) return PSA_ERROR_INVALID_ARGUMENT;
        switch (bits) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255)
        case 255: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_255 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448) || \
    defined(PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448)
        case 448: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS_448 ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS_448 */
        default: return PSA_ERROR_NOT_SUPPORTED;
        }
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_TWISTED_EDWARDS ||
          PSA_NEED_OBERON_KEY_TYPE_ECC_PUBLIC_KEY_TWISTED_EDWARDS */

    default:
        (void)res;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (*key_bits != 0 && *key_bits != bits) return PSA_ERROR_INVALID_ARGUMENT;
    if (key_size < data_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(key, data, data_length);
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY)
    if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)) {
        oberon_set_forced_bits(key, bits);
    }
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_IMPORT_MONTGOMERY */
    *key_length = data_length;
    *key_bits = bits;
    return PSA_SUCCESS;
}

psa_status_t oberon_generate_ec_key(
    const psa_key_attributes_t *attributes,
    uint8_t *key, size_t key_size, size_t *key_length)
{
    int res;
    psa_status_t status;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);
    size_t length = PSA_BITS_TO_BYTES(bits);

    switch (type) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (key_size < length) return PSA_ERROR_BUFFER_TOO_SMALL;
        do {
            do {
                status = psa_generate_random(key, length);
                if (status) return status;
            } while (oberon_ct_compare_zero(key, length) == 0);
            switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_224
            case 224:
                res = ocrypto_ecdh_p224_secret_key_check(key);
                break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_256
            case 256:
                res = ocrypto_ecdh_p256_secret_key_check(key);
                break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_384
            case 384:
                res = ocrypto_ecdh_p384_secret_key_check(key);
                break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_521
            case 521:
                res = ocrypto_ecdh_p521_secret_key_check(key);
                break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP_R1_521 */
            default:
                return PSA_ERROR_NOT_SUPPORTED;
            }
        } while (res);
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_SECP */

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
        if (key_size < length) return PSA_ERROR_BUFFER_TOO_SMALL;
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255
        case 255: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_255 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448
        case 448: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY_448 */
        default: return PSA_ERROR_NOT_SUPPORTED;
        }
        status = psa_generate_random(key, length);
        if (status) return status;
        oberon_set_forced_bits(key, bits);
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_MONTGOMERY */

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        length = PSA_BITS_TO_BYTES(bits + 1); // ED needs an extra bit
        if (key_size < length) return PSA_ERROR_BUFFER_TOO_SMALL;
        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_255
        case 255: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_255 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_448
        case 448: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS_448 */
        default: return PSA_ERROR_NOT_SUPPORTED;
        }
        status = psa_generate_random(key, length);
        if (status) return status;
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_GENERATE_TWISTED_EDWARDS */

    default:
        (void)key;
        (void)res;
        (void)status;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    *key_length = length;
    return PSA_SUCCESS;
}

psa_status_t oberon_derive_ec_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *input, size_t input_length,
    uint8_t *key, size_t key_size, size_t *key_length)
{
    int res;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP
    uint32_t c;
    size_t i;
#endif

    if (key_size < input_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(key, input, input_length);
    *key_length = input_length;

    // check and preprocess key data
    switch (type) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):

        // increment key data
        c = 1; i = input_length;
        do {
            c += key[--i];
            key[i] = (uint8_t)c;
            c >>= 8;
        } while (i > 0);

        switch (bits) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_224
        case 224:
            res = ocrypto_ecdh_p224_secret_key_check(key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_256
        case 256:
            res = ocrypto_ecdh_p256_secret_key_check(key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_384
        case 384:
            res = ocrypto_ecdh_p384_secret_key_check(key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_521
        case 521:
            key[0] &= 0x01; // truncate to 521 bits
            res = ocrypto_ecdh_p521_secret_key_check(key);
            break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP_R1_521 */
        default:
            return PSA_ERROR_INVALID_ARGUMENT;
        }
        // repeat if input out of range
        if (res || !oberon_ct_compare_zero(key, input_length)) return PSA_ERROR_INSUFFICIENT_DATA;
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_SECP */

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY):
        switch (bits) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_255)
        case 255: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_448)
        case 448: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY_448 */
        default: return PSA_ERROR_INVALID_ARGUMENT;
        }
        oberon_set_forced_bits(key, bits);
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_MONTGOMERY */

#ifdef PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        switch (bits) {
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_255)
        case 255: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_255 */
#if defined(PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_448)
        case 448: break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS_448 */
        default: return PSA_ERROR_INVALID_ARGUMENT;
        }
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_DERIVE_TWISTED_EDWARDS */

    default:
        (void)res;
        (void)bits;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}
