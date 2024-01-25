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
#include "oberon_ecdsa.h"
#include "psa_crypto_driver_wrappers.h"

#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_224
#include "ocrypto_ecdsa_p224.h"
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_256
#include "ocrypto_ecdsa_p256.h"
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_384
#include "ocrypto_ecdsa_p384.h"
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_521
#include "ocrypto_ecdsa_p521.h"
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_521 */
#ifdef PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255
#include "ocrypto_ed25519.h"
#endif /* PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255 */
#ifdef PSA_NEED_OBERON_ED25519PH
#include "ocrypto_ed25519ph.h"
#endif /* PSA_NEED_OBERON_ED25519PH */
#ifdef PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448
#include "ocrypto_ed448.h"
#endif /* PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448 */
#ifdef PSA_NEED_OBERON_ED448PH
#include "ocrypto_ed448ph.h"
#endif /* PSA_NEED_OBERON_ED448PH */


#ifdef PSA_NEED_OBERON_ECDSA_SIGN
static int ecdsa_sign_hash(
    const uint8_t *key, size_t key_length,
    const uint8_t *hash,
    const uint8_t *ek,
    uint8_t *signature)
{
    int res;

    switch (key_length) {
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_224
    case PSA_BITS_TO_BYTES(224):
        res = ocrypto_ecdsa_p224_sign_hash(signature, hash, key, ek);
        break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_256
    case PSA_BITS_TO_BYTES(256):
        res = ocrypto_ecdsa_p256_sign_hash(signature, hash, key, ek);
        break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_384
    case PSA_BITS_TO_BYTES(384):
        res = ocrypto_ecdsa_p384_sign_hash(signature, hash, key, ek);
        break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_521
    case PSA_BITS_TO_BYTES(521):
        res = ocrypto_ecdsa_p521_sign_hash(signature, hash, key, ek);
        break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_521 */
    default:
        (void)key;
        (void)hash;
        (void)ek;
        (void)signature;
        res = 1;
    }

    return res;
}
#endif /* PSA_NEED_OBERON_ECDSA_SIGN */

#ifdef PSA_NEED_OBERON_ECDSA_DETERMINISTIC
/* hmac = HMAC_key(v || tag || sk || ext_hash) */
static psa_status_t deterministic_ecdsa_hmac(
    psa_algorithm_t hmac_alg,
    const psa_key_attributes_t *attr,
    const uint8_t *key, const uint8_t *v, size_t hash_len,
    uint8_t tag,
    const uint8_t *sk, const uint8_t *hash, size_t key_len,
    uint8_t *hmac)
{
    psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
    psa_status_t status;
    size_t length;

    status = psa_driver_wrapper_mac_sign_setup(
        &operation,
        attr, key, hash_len,
        hmac_alg);
    if (status) goto exit;

    status = psa_driver_wrapper_mac_update(&operation, v, hash_len);
    if (status) goto exit;

    if (tag != 0xFF) {
        status = psa_driver_wrapper_mac_update(&operation, &tag, 1);
        if (status) goto exit;
    }

    if (sk) {
        status = psa_driver_wrapper_mac_update(&operation, sk, key_len);
        if (status) goto exit;
    }

    if (hash) {
        status = psa_driver_wrapper_mac_update(&operation, hash, key_len);
        if (status) goto exit;
    }

    return psa_driver_wrapper_mac_sign_finish(&operation, hmac, hash_len, &length);

exit:
    psa_driver_wrapper_mac_abort(&operation);
    return status;
}

static psa_status_t deterministic_ecdsa_sign_hash(
    psa_algorithm_t hash_alg,
    const uint8_t *hash, // size: key_length
    const uint8_t *key, size_t key_length,
    uint8_t *ek, // size: key_length
    uint8_t *signature)
{
    size_t hash_length = PSA_HASH_LENGTH(hash_alg);
    uint8_t K[PSA_HASH_MAX_SIZE];
    uint8_t V[PSA_HASH_MAX_SIZE];
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_algorithm_t hmac_alg = PSA_ALG_HMAC(hash_alg);
    psa_status_t status;
    size_t len;
    int res;

    if (hash_length == 0) return PSA_ERROR_INVALID_ARGUMENT;

    // calculate deterministic session key
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attr, hmac_alg);
    psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(hash_length));

    memset(V, 0x01, hash_length); // set all V bytes to 0x01  // b
    memset(K, 0x00, hash_length); // set all K bytes to 0x00  // c

    status = deterministic_ecdsa_hmac(hmac_alg, &attr, K, V, hash_length, 0x00, key, hash, key_length, K); // d
    if (status) return status;
    status = deterministic_ecdsa_hmac(hmac_alg, &attr, K, V, hash_length, 0xFF, NULL, NULL, 0, V); // e
    if (status) return status;
    status = deterministic_ecdsa_hmac(hmac_alg, &attr, K, V, hash_length, 0x01, key, hash, key_length, K); // f
    if (status) return status;

    while (1) {
        status = deterministic_ecdsa_hmac(hmac_alg, &attr, K, V, hash_length, 0xFF, NULL, NULL, 0, V); // g, h3
        if (status) return status;
        for (len = 0; len < key_length; len += hash_length) {
            status = deterministic_ecdsa_hmac(hmac_alg, &attr, K, V, hash_length, 0xFF, NULL, NULL, 0, V); // h2
            if (status) return status;
            memcpy(ek + len, V, key_length - len < hash_length ? key_length - len : hash_length); // T = T || V
        }

        res = ecdsa_sign_hash(key, key_length, hash, ek, signature);
        if (res > 0) return PSA_ERROR_NOT_SUPPORTED;
        if (res == 0) break; // 0 < T < prime

        // T out of range: retry
        status = deterministic_ecdsa_hmac(hmac_alg, &attr, K, V, hash_length, 0x00, NULL, NULL, 0, K); // h3
        if (status) return status;
    }

    return PSA_SUCCESS;
}
#endif /* PSA_NEED_OBERON_ECDSA_DETERMINISTIC */

psa_status_t oberon_ecdsa_sign_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length)
{
    int res;
    psa_status_t status;
    uint8_t ek[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)];
#if defined(PSA_NEED_OBERON_ECDSA_RANDOMIZED) || defined(PSA_NEED_OBERON_ECDSA_DETERMINISTIC)
    uint8_t ext_hash[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)];
    size_t bits = psa_get_key_bits(attributes);
#endif

    switch (psa_get_key_type(attributes)) {
#if defined(PSA_NEED_OBERON_ECDSA_RANDOMIZED) || defined(PSA_NEED_OBERON_ECDSA_DETERMINISTIC)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (hash_length == 0 || key_length != PSA_BITS_TO_BYTES(bits)) return PSA_ERROR_INVALID_ARGUMENT;
        if (signature_size < key_length * 2) return PSA_ERROR_BUFFER_TOO_SMALL;
        *signature_length = key_length * 2;

        if (hash_length < key_length) {
            if (key_length > sizeof ext_hash) return PSA_ERROR_INSUFFICIENT_MEMORY;
            // add most significant zero bits
            memset(ext_hash, 0, key_length - hash_length);
            memcpy(ext_hash + key_length - hash_length, hash, hash_length);
            hash = ext_hash;
        }

#ifdef PSA_NEED_OBERON_ECDSA_RANDOMIZED
        if (PSA_ALG_IS_RANDOMIZED_ECDSA(alg)) {
            do {
                status = psa_generate_random(ek, key_length); // ephemeral key
                if (status != PSA_SUCCESS) return status;
                res = ecdsa_sign_hash(key, key_length, hash, ek, signature);
                if (res > 0) return PSA_ERROR_NOT_SUPPORTED;
            } while (res != 0);
        } else
#endif /* PSA_NEED_OBERON_ECDSA_RANDOMIZED */
#ifdef PSA_NEED_OBERON_ECDSA_DETERMINISTIC
        if (PSA_ALG_IS_DETERMINISTIC_ECDSA(alg)) {
            psa_algorithm_t hash_alg = PSA_ALG_SIGN_GET_HASH(alg);
            return deterministic_ecdsa_sign_hash(hash_alg, hash, key, key_length, ek, signature);
        } else
#endif /* PSA_NEED_OBERON_ECDSA_DETERMINISTIC */
        {
            return PSA_ERROR_INVALID_ARGUMENT; //  PSA_ERROR_NOT_SUPPORTED;
        }
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_ECDSA_RANDOMIZED || PSA_NEED_OBERON_ECDSA_DETERMINISTIC */

#if defined(PSA_NEED_OBERON_ED25519PH) || defined(PSA_NEED_OBERON_ED448PH)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        switch (psa_get_key_bits(attributes)) {
#ifdef PSA_NEED_OBERON_ED25519PH
        case 255:
            if (hash_length != ocrypto_ed25519ph_HASH_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (key_length != ocrypto_ed25519ph_SECRET_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_size < ocrypto_ed25519ph_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
            *signature_length = ocrypto_ed25519ph_BYTES;
            ocrypto_ed25519ph_public_key(ek, key); // calculate public key
            ocrypto_ed25519ph_sign(signature, hash, key, ek);
            break;
#endif /* PSA_NEED_OBERON_ED25519PH */
#ifdef PSA_NEED_OBERON_ED448PH
        case 448:
            if (hash_length != ocrypto_ed448ph_HASH_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (key_length != ocrypto_ed448ph_SECRET_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_size < ocrypto_ed448ph_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
            *signature_length = ocrypto_ed448ph_BYTES;
            ocrypto_ed448ph_public_key(ek, key); // calculate public key
            ocrypto_ed448ph_sign(signature, hash, key, ek);
            break;
#endif /* PSA_NEED_OBERON_ED448PH */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_ED25519PH || PSA_NEED_OBERON_ED448PH */

    default:
        (void)key;
        (void)alg;
        (void)signature;
        (void)ek;
        (void)status;
        (void)res;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_ecdsa_sign_message(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    uint8_t *signature, size_t signature_size, size_t *signature_length)
{
#if defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448)
    uint8_t pub_key[57];
#elif defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255)
    uint8_t pub_key[32];
#endif
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
#if defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255) || defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        // EDDSA is only available in sign_message
        switch (psa_get_key_bits(attributes)) {
#ifdef PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255
        case 255:
            if (key_length != ocrypto_ed25519_SECRET_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_size < ocrypto_ed25519_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
            *signature_length = ocrypto_ed25519_BYTES;
            ocrypto_ed25519_public_key(pub_key, key); // calculate public key
            ocrypto_ed25519_sign(signature, input, input_length, key, pub_key);
            break;
#endif
#ifdef PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448
        case 448:
            if (key_length != ocrypto_ed448_SECRET_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_size < ocrypto_ed448_BYTES) return PSA_ERROR_BUFFER_TOO_SMALL;
            *signature_length = ocrypto_ed448_BYTES;
            ocrypto_ed448_public_key(pub_key, key); // calculate public key
            ocrypto_ed448_sign(signature, input, input_length, key, pub_key);
            break;
#endif
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255 || PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448 */
    default:
        (void)key;
        (void)key_length;
        (void)alg;
        (void)input;
        (void)input_length;
        (void)signature;
        (void)signature_size;
        (void)signature_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_ecdsa_verify_hash(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *hash, size_t hash_length,
    const uint8_t *signature, size_t signature_length)
{
    int res;
    uint8_t key_buf[2 * PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)];
#if defined(PSA_NEED_OBERON_ECDSA_RANDOMIZED) || defined(PSA_NEED_OBERON_ECDSA_DETERMINISTIC)
    uint8_t ext_hash[PSA_BITS_TO_BYTES(PSA_VENDOR_ECC_MAX_CURVE_BITS)];
    const uint8_t *pub_key;
    size_t bits = psa_get_key_bits(attributes);
    size_t length = PSA_BITS_TO_BYTES(bits);
#endif
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
#if defined(PSA_NEED_OBERON_ECDSA_RANDOMIZED) || defined(PSA_NEED_OBERON_ECDSA_DETERMINISTIC)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
    case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1):
        if (hash_length == 0) return PSA_ERROR_INVALID_ARGUMENT;
        if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
            if (key_length != length) return PSA_ERROR_INVALID_ARGUMENT;
            pub_key = key_buf;
        } else {
            if (key_length != length * 2 + 1 || key[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
            pub_key = &key[1];
        }

        if (hash_length < length) {
            if (length > sizeof ext_hash) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memset(ext_hash, 0, length - hash_length);
            memcpy(ext_hash + length - hash_length, hash, hash_length);
            hash = ext_hash;
        }

        if (PSA_ALG_IS_ECDSA(alg)) {
            if (signature_length != 2 * length) return PSA_ERROR_INVALID_SIGNATURE;
            switch (length) {
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_224
            case PSA_BITS_TO_BYTES(224):
                if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
                    ocrypto_ecdsa_p224_public_key(key_buf, key);
                }
                res = ocrypto_ecdsa_p224_verify_hash(signature, hash, pub_key);
                break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_224 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_256
            case PSA_BITS_TO_BYTES(256):
                if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
                    ocrypto_ecdsa_p256_public_key(key_buf, key);
                }
                res = ocrypto_ecdsa_p256_verify_hash(signature, hash, pub_key);
                break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_256 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_384
            case PSA_BITS_TO_BYTES(384):
                if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
                    ocrypto_ecdsa_p384_public_key(key_buf, key);
                }
                res = ocrypto_ecdsa_p384_verify_hash(signature, hash, pub_key);
                break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_384 */
#ifdef PSA_NEED_OBERON_ECDSA_SECP_R1_521
            case PSA_BITS_TO_BYTES(521):
                if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)) {
                    ocrypto_ecdsa_p521_public_key(key_buf, key);
                }
                res = ocrypto_ecdsa_p521_verify_hash(signature, hash, pub_key);
                break;
#endif /* PSA_NEED_OBERON_ECDSA_SECP_R1_521 */
            default:
                (void)signature;
                return PSA_ERROR_NOT_SUPPORTED;
            }
        } else {
            return PSA_ERROR_INVALID_ARGUMENT; //  PSA_ERROR_NOT_SUPPORTED;
        }
        if (res != 0) return PSA_ERROR_INVALID_SIGNATURE;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_ECDSA_RANDOMIZED || PSA_NEED_OBERON_ECDSA_DETERMINISTIC */

#if defined(PSA_NEED_OBERON_ED25519PH) || defined(PSA_NEED_OBERON_ED448PH)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
    case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        switch (psa_get_key_bits(attributes)) {
#ifdef PSA_NEED_OBERON_ED25519PH
        case 255:
            if (key_length != ocrypto_ed25519ph_PUBLIC_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (hash_length != ocrypto_ed25519ph_HASH_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_length != ocrypto_ed25519ph_BYTES) return PSA_ERROR_INVALID_SIGNATURE;
            if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)) {
                ocrypto_ed25519ph_public_key(key_buf, key);
                key = key_buf;
            }
            res = ocrypto_ed25519ph_verify(signature, hash, key);
            break;
#endif /* PSA_NEED_OBERON_ED25519PH */
#ifdef PSA_NEED_OBERON_ED448PH
        case 448:
            if (key_length != ocrypto_ed448ph_PUBLIC_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (hash_length != ocrypto_ed448ph_HASH_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_length != ocrypto_ed448ph_BYTES) return PSA_ERROR_INVALID_SIGNATURE;
            if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)) {
                ocrypto_ed448ph_public_key(key_buf, key);
                key = key_buf;
            }
            res = ocrypto_ed448ph_verify(signature, hash, key);
            break;
#endif /* PSA_NEED_OBERON_ED448PH */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        if (res) return PSA_ERROR_INVALID_SIGNATURE;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_ED25519PH || PSA_NEED_OBERON_ED448PH */

    default:
        (void)key;
        (void)key_length;
        (void)alg;
        (void)hash;
        (void)hash_length;
        (void)signature;
        (void)signature_length;
        (void)res;
        (void)key_buf;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_ecdsa_verify_message(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    const uint8_t *signature, size_t signature_length)
{
    int res;
#if defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448)
    uint8_t pub_key[57];
#elif defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255)
    uint8_t pub_key[32];
#endif
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
#if defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255) || defined(PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448)
    case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS):
    case PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS):
        // EDDSA is only available in verify_message
        switch (psa_get_key_bits(attributes)) {
#ifdef PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255
        case 255:
            if (key_length != ocrypto_ed25519_PUBLIC_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_length != ocrypto_ed25519_BYTES) return PSA_ERROR_INVALID_SIGNATURE;
            if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)) {
                ocrypto_ed25519_public_key(pub_key, key);
                key = pub_key;
            }
            res = ocrypto_ed25519_verify(signature, input, input_length, key);
            break;
#endif /* PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255 */
#ifdef PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448
        case 448:
            if (key_length != ocrypto_ed448_PUBLIC_KEY_BYTES) return PSA_ERROR_INVALID_ARGUMENT;
            if (signature_length != ocrypto_ed448_BYTES) return PSA_ERROR_INVALID_SIGNATURE;
            if (type == PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)) {
                ocrypto_ed448_public_key(pub_key, key);
                key = pub_key;
            }
            res = ocrypto_ed448_verify(signature, input, input_length, key);
            break;
#endif /* PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448 */
        default:
            return PSA_ERROR_NOT_SUPPORTED;
        }
        if (res) return PSA_ERROR_INVALID_SIGNATURE;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_255 || PSA_NEED_OBERON_PURE_EDDSA_TWISTED_EDWARDS_448 */
    default:
        (void)key;
        (void)key_length;
        (void)alg;
        (void)input;
        (void)input_length;
        (void)signature;
        (void)signature_length;
        (void)res;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}
