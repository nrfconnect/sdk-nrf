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
#include "oberon_key_derivation.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"


#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128)
static const uint8_t zero[PSA_HASH_MAX_SIZE] = { 0 };
#endif


#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128) || \
    defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
static psa_status_t oberon_setup_mac(
    oberon_key_derivation_operation_t *operation,
    const uint8_t *key, size_t key_length)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, operation->key_type);
    psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(key_length));
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    
    memset(&operation->mac_op, 0, sizeof operation->mac_op);
    return psa_driver_wrapper_mac_sign_setup(
        &operation->mac_op,
        &attributes, key, key_length,
        operation->mac_alg);
}
#endif

#if defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128) || \
    defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
static psa_status_t oberon_mac_update_num(
    oberon_key_derivation_operation_t *operation,
    uint32_t num)
{
    uint8_t idx[4];

    idx[0] = (uint8_t)(num >> 24);
    idx[1] = (uint8_t)(num >> 16);
    idx[2] = (uint8_t)(num >> 8);
    idx[3] = (uint8_t)(num);
    return psa_driver_wrapper_mac_update(&operation->mac_op, idx, 4);
}
#endif


#ifdef PSA_NEED_OBERON_PBKDF2_HMAC
static psa_status_t oberon_hash_key(
    oberon_key_derivation_operation_t *operation,
    const uint8_t *data, size_t data_length)
{
    psa_status_t status;
    size_t length;

    memset(&operation->hash_op, 0, sizeof operation->hash_op);
    status = psa_driver_wrapper_hash_setup(&operation->hash_op, PSA_ALG_GET_HASH(operation->mac_alg));
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&operation->hash_op, data, data_length);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&operation->hash_op, operation->key, operation->block_length, &length);

exit:
    psa_driver_wrapper_hash_abort(&operation->hash_op);
    return status;
}
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC */


psa_status_t oberon_key_derivation_setup(
    oberon_key_derivation_operation_t *operation,
    psa_algorithm_t alg)
{
#ifdef PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128
    if (alg == PSA_ALG_PBKDF2_AES_CMAC_PRF_128) {
        operation->block_length = PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES);
        operation->mac_alg = PSA_ALG_CMAC;
        operation->key_type = PSA_KEY_TYPE_AES;
        operation->alg = OBERON_PBKDF2_CMAC_ALG;
    } else
#endif /* PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 */

#ifdef PSA_NEED_OBERON_SP800_108_COUNTER_CMAC
    if (alg == PSA_ALG_SP800_108_COUNTER_CMAC) {
        operation->block_length = PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES);
        operation->mac_alg = PSA_ALG_CMAC;
        operation->key_type = PSA_KEY_TYPE_AES;
        operation->alg = OBERON_SP800_108_COUNTER_ALG;
        operation->info[0] = 0u; // separator
        operation->info_length = 1;
        operation->count = 0xFFFFFFF8;
    } else
#endif /* PSA_NEED_OBERON_SP800_108_COUNTER_CMAC */

#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
    if (PSA_ALG_IS_SRP_PASSWORD_HASH(alg)) {
        operation->alg = OBERON_SRP_PASSWORD_HASH_ALG;
        operation->mac_alg = PSA_ALG_HKDF_GET_HASH(alg);
    } else
#endif /* PSA_NEED_OBERON_SRP_PASSWORD_HASH */

    {
        // all olthers are HMAC based
        psa_algorithm_t hash = PSA_ALG_HKDF_GET_HASH(alg);
        unsigned hash_length = PSA_HASH_LENGTH(hash);
        if (hash_length == 0) return PSA_ERROR_NOT_SUPPORTED;
        operation->block_length = (uint8_t)hash_length;
        operation->mac_alg = PSA_ALG_HMAC(hash);
        operation->key_type = PSA_KEY_TYPE_HMAC;

#ifdef PSA_NEED_OBERON_HKDF
        if (PSA_ALG_IS_HKDF(alg)) {
            operation->info_length = 0;
            operation->alg = OBERON_HKDF_ALG;
        } else
#endif /* PSA_NEED_OBERON_HKDF */

#ifdef PSA_NEED_OBERON_HKDF_EXTRACT
        if (PSA_ALG_IS_HKDF_EXTRACT(alg)) {
            operation->info_length = 0;
            operation->alg = OBERON_HKDF_EXTRACT_ALG;
        } else
#endif /* PSA_NEED_OBERON_HKDF_EXTRACT */

#ifdef PSA_NEED_OBERON_HKDF_EXPAND
        if (PSA_ALG_IS_HKDF_EXPAND(alg)) {
            psa_algorithm_t hash = PSA_ALG_HKDF_GET_HASH(alg);
            unsigned hash_length = PSA_HASH_LENGTH(hash);
            if (hash_length == 0) return PSA_ERROR_NOT_SUPPORTED;
            operation->info_length = 0;
            operation->alg = OBERON_HKDF_EXPAND_ALG;
        } else
#endif /* PSA_NEED_OBERON_HKDF_EXPAND */

#ifdef PSA_NEED_OBERON_TLS12_PRF
        if (PSA_ALG_IS_TLS12_PRF(alg)) {
            operation->alg = OBERON_TLS12_PRF_ALG;
        } else
#endif /* PSA_NEED_OBERON_TLS12_PRF */

#ifdef PSA_NEED_OBERON_TLS12_PSK_TO_MS
        if (PSA_ALG_IS_TLS12_PSK_TO_MS(alg)) {
            operation->alg = OBERON_TLS12_PSK_TO_MS_ALG;
            operation->count = 0;
        } else
#endif /* PSA_NEED_OBERON_TLS12_PSK_TO_MS */

#ifdef PSA_NEED_OBERON_PBKDF2_HMAC
        if (PSA_ALG_IS_PBKDF2_HMAC(alg)) {
            operation->alg = OBERON_PBKDF2_HMAC_ALG;
        } else
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC */

#ifdef PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS
        if (alg == PSA_ALG_TLS12_ECJPAKE_TO_PMS) {
            operation->alg = OBERON_ECJPAKE_TO_PMS_ALG;
        } else
#endif /* PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS */

#ifdef PSA_NEED_OBERON_SP800_108_COUNTER_HMAC
        if (PSA_ALG_IS_SP800_108_COUNTER_HMAC(alg)) {
            operation->alg = OBERON_SP800_108_COUNTER_ALG;
            operation->info[0] = 0u; // separator
            operation->info_length = 1;
            operation->count = 0xFFFFFFF8;
        } else
#endif /* PSA_NEED_OBERON_SP800_108_COUNTER_HMAC */

        {
            (void)alg;
            return PSA_ERROR_NOT_SUPPORTED;
        }
    }

    operation->salt_length = 0;
    operation->data_length = 0;
    operation->index = 1;
    return PSA_SUCCESS;
}

psa_status_t oberon_key_derivation_set_capacity(
    oberon_key_derivation_operation_t *operation,
    size_t capacity)
{
#if defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
    if (operation->alg == OBERON_SP800_108_COUNTER_ALG) {
        operation->count = (uint32_t)(capacity * 8); // L in bits
    }
#endif
    (void)operation;
    (void)capacity;
    return PSA_SUCCESS;
}

psa_status_t oberon_key_derivation_input_bytes(
    oberon_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    const uint8_t *data, size_t data_length)
{
    psa_status_t status;
    size_t i, length;

    switch (step) {

    case PSA_KEY_DERIVATION_INPUT_SALT:
        if (data_length) {
            if (operation->alg == OBERON_HKDF_ALG || operation->alg == OBERON_HKDF_EXTRACT_ALG) {
#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXTRACT)
                status = oberon_setup_mac(operation, data, data_length);
                if (status) goto exit;
                operation->salt_length = (uint16_t)data_length;
#endif /* PSA_NEED_OBERON_HKDF || PSA_NEED_OBERON_HKDF_EXTRACT */
#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
            } else if (operation->alg == OBERON_SRP_PASSWORD_HASH_ALG) {
                status = psa_driver_wrapper_hash_finish(&operation->hash_op, operation->data, sizeof operation->data, &length);
                if (status) goto exit;
                status = psa_driver_wrapper_hash_setup(&operation->hash_op, PSA_ALG_GET_HASH(operation->mac_alg));
                if (status) goto exit;
                status = psa_driver_wrapper_hash_update(&operation->hash_op, data, data_length); // salt
                if (status) goto exit;
                status = psa_driver_wrapper_hash_update(&operation->hash_op, operation->data, length); // H(u, ":", pw)
                if (status) goto exit;
#endif /* PSA_NEED_OBERON_SRP_PASSWORD_HASH */
            } else {
#if defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128)
                length = operation->salt_length + data_length;
                if (length < data_length || length > sizeof operation->info) return PSA_ERROR_INSUFFICIENT_MEMORY;
                memcpy(operation->info + operation->salt_length, data, data_length);
                operation->salt_length = (uint16_t)length;
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC || PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 */
            }
        }
        return PSA_SUCCESS;

#if defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS)
    case PSA_KEY_DERIVATION_INPUT_OTHER_SECRET:
        if (data_length) {
            if (data_length > sizeof operation->key - PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE - 4) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memcpy(operation->key + 2, data, data_length);
        }
        operation->key_length = (uint16_t)data_length;
        operation->count = 1;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_TLS12_PSK_TO_MS */

#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXTRACT) || \
    defined(PSA_NEED_OBERON_HKDF_EXPAND) || defined(PSA_NEED_OBERON_TLS12_PRF) || \
    defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS) || defined(PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS) || \
    defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
    case PSA_KEY_DERIVATION_INPUT_SECRET:
        switch (operation->alg) {
#ifdef PSA_NEED_OBERON_HKDF_EXPAND
        case OBERON_HKDF_EXPAND_ALG:
            // skip extract phase
            if (data_length != operation->block_length) return PSA_ERROR_INVALID_ARGUMENT;
            memcpy(operation->key, data, data_length);
            return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_HKDF_EXPAND */
#ifdef PSA_NEED_OBERON_TLS12_PRF
        case OBERON_TLS12_PRF_ALG:
            if (data_length > sizeof operation->key) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memcpy(operation->key, data, data_length);
            operation->key_length = (uint16_t)data_length;
            return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_TLS12_PRF */
#ifdef PSA_NEED_OBERON_TLS12_PSK_TO_MS
        case OBERON_TLS12_PSK_TO_MS_ALG:
            if (data_length > PSA_TLS12_PSK_TO_MS_PSK_MAX_SIZE) return PSA_ERROR_INVALID_ARGUMENT;
            length = operation->key_length; // other secret length
            if (operation->count == 0) {
                // plain PSK: set other secret to data_length zeroes
                memset(operation->key + 2, 0, data_length);
                length = data_length;
            }
            operation->key[0] = (uint8_t)(length >> 8);
            operation->key[1] = (uint8_t)length;
            operation->key[length + 2] = (uint8_t)(data_length >> 8);
            operation->key[length + 3] = (uint8_t)data_length;
            memcpy(operation->key + length + 4, data, data_length);
            operation->key_length = (uint16_t)(length + data_length + 4); // total secret length
            return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_TLS12_PSK_TO_MS */
#ifdef PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS
        case OBERON_ECJPAKE_TO_PMS_ALG:
            if (data_length != PSA_TLS12_ECJPAKE_TO_PMS_INPUT_SIZE || data[0] != 0x04) {
                return PSA_ERROR_INVALID_ARGUMENT; // P256 point
            }
            memcpy(operation->key, &data[1], 32); // input.x
            operation->key_length = (uint16_t)32;
            return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS */
#if defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
        case OBERON_SP800_108_COUNTER_ALG:
            if (data_length > sizeof operation->key) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memcpy(operation->key, data, data_length);
            operation->key_length = (uint16_t)data_length;
            return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_SP800_108_COUNTER_HMAC || PSA_NEED_OBERON_SP800_108_COUNTER_CMAC */
        default:
#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXTRACT)
            if (operation->salt_length == 0) {
                // set zero salt
                status = oberon_setup_mac(operation, zero, operation->block_length);
                if (status) goto exit;
            }
            // add secret
            status = psa_driver_wrapper_mac_update(&operation->mac_op, data, data_length);
            if (status) goto exit;
            // HKDF extract
            status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op,
                operation->key, operation->block_length, &length);
            if (status) goto exit;
#endif /* PSA_NEED_OBERON_HKDF || PSA_NEED_OBERON_HKDF_EXTRACT */
            return PSA_SUCCESS;
        }
#endif /* PSA_NEED_OBERON_HKDF || PSA_NEED_OBERON_HKDF_EXTRACT || PSA_NEED_OBERON_HKDF_EXPAND || PSA_NEED_OBERON_TLS12 */

#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXTRACT) || defined(PSA_NEED_OBERON_HKDF_EXPAND) || \
    defined(PSA_NEED_OBERON_SRP_PASSWORD_HASH)
    case PSA_KEY_DERIVATION_INPUT_INFO:
#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
        if (operation->alg == OBERON_SRP_PASSWORD_HASH_ALG) {
            status = psa_driver_wrapper_hash_setup(&operation->hash_op, PSA_ALG_GET_HASH(operation->mac_alg));
            if (status) goto exit;
            status = psa_driver_wrapper_hash_update(&operation->hash_op, data, data_length); // user id
            if (status) goto exit;
            return PSA_SUCCESS;
        } else
#endif
        {
            if (data_length > sizeof operation->info) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memcpy(operation->info, data, data_length);
            operation->info_length = (uint16_t)data_length;
            return PSA_SUCCESS;
        }
#endif /* PSA_NEED_OBERON_HKDF || PSA_NEED_OBERON_HKDF_EXTRACT || PSA_NEED_OBERON_HKDF_EXPAND ||
          PSA_NEED_OBERON_SRP_PASSWORD_HASH */

#if defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128) || \
    defined(PSA_NEED_OBERON_SRP_PASSWORD_HASH)
    case PSA_KEY_DERIVATION_INPUT_PASSWORD:
        switch (operation->alg) {
#ifdef PSA_NEED_OBERON_PBKDF2_HMAC
        case OBERON_PBKDF2_HMAC_ALG:
            if (data_length > PSA_HASH_BLOCK_LENGTH(operation->mac_alg)) {
                // key = H(password)
                status = oberon_hash_key(operation, data, data_length);
                if (status) return status; // no cleanup needed
                operation->key_length = (uint16_t)operation->block_length;
            } else {
                memcpy(operation->key, data, data_length);
                operation->key_length = (uint16_t)data_length;
            }
            break;
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC */
#ifdef PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128
        case OBERON_PBKDF2_CMAC_ALG:
            if (data_length == 16) {
                memcpy(operation->key, data, 16);
            } else {
                // key = mac(zero, password)
                status = oberon_setup_mac(operation, zero, 16); // zero key
                if (status) goto exit;
                status = psa_driver_wrapper_mac_update(&operation->mac_op, data, data_length);
                if (status) goto exit;
                status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, operation->key, 16, &length);
                if (status) goto exit;
            }
            operation->key_length = 16;
            break;
#endif /* PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 */
#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
        case OBERON_SRP_PASSWORD_HASH_ALG:
            status = psa_driver_wrapper_hash_update(&operation->hash_op, (const uint8_t *)":", 1); // ":"
            if (status) goto exit;
            status = psa_driver_wrapper_hash_update(&operation->hash_op, data, data_length); // pw
            if (status) goto exit;
            break;
#endif /* PSA_NEED_OBERON_SRP_PASSWORD_HASH */
        default:
            break;
        }
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC || PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 || PSA_NEED_OBERON_SRP_PASSWORD_HASH */

#if defined(PSA_NEED_OBERON_TLS12_PRF) || defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS)
    case PSA_KEY_DERIVATION_INPUT_SEED:
        if (data_length > sizeof operation->info) return PSA_ERROR_INSUFFICIENT_MEMORY;
        memcpy(operation->info, data, data_length);
        operation->info_length = (uint16_t)data_length;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_TLS12_PRF || PSA_NEED_OBERON_TLS12_PSK_TO_MS */

#if defined(PSA_NEED_OBERON_TLS12_PRF) || defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS) || \
    defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
    case PSA_KEY_DERIVATION_INPUT_LABEL:
#if defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
        if (operation->alg == OBERON_SP800_108_COUNTER_ALG) {
            for (i = 0; i < data_length; i++) {
                if (data[i] == 0) return PSA_ERROR_INVALID_ARGUMENT;
            }
            // store label
            if (data_length >= sizeof operation->info) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memcpy(operation->info, data, data_length);
            operation->info[data_length] = 0u; // separator
            operation->info_length = (uint8_t)data_length + 1;
            return PSA_SUCCESS;
        } else
#endif /* PSA_NEED_OBERON_SP800_108_COUNTER_HMAC || PSA_NEED_OBERON_SP800_108_COUNTER_CMAC */
        {
#if defined(PSA_NEED_OBERON_TLS12_PRF) || defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS)
            // TLS12
            // seed = label || seed
            length = operation->info_length + data_length;
            if (length < data_length || length > sizeof operation->info) return PSA_ERROR_INSUFFICIENT_MEMORY;
            memmove(operation->info + data_length, operation->info, operation->info_length);
            memcpy(operation->info, data, data_length);
            operation->info_length = (uint16_t)length;
#endif /* PSA_NEED_OBERON_TLS12_PRF || PSA_NEED_OBERON_TLS12_PSK_TO_MS */
            return PSA_SUCCESS;
        }
#endif /* PSA_NEED_OBERON_TLS12_PRF || PSA_NEED_OBERON_TLS12_PSK_TO_MS || PSA_NEED_OBERON_SP800_108_COUNTER */

#if defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
    case PSA_KEY_DERIVATION_INPUT_CONTEXT:
        // insert context
        length = operation->info_length + data_length;
        if (length < data_length || length > sizeof operation->info) return PSA_ERROR_INSUFFICIENT_MEMORY;
        memcpy(operation->info + operation->info_length, data, data_length);
        operation->info_length = (uint16_t)length;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_SP800_108_COUNTER_HMAC || PSA_NEED_OBERON_SP800_108_COUNTER_CMAC */

    default:
        (void)data;
        (void)data_length;
        (void)status;
        (void)length;
        (void)i;
        return PSA_ERROR_INVALID_ARGUMENT;
    }

#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXTRACT) || \
    defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128) || defined(PSA_NEED_OBERON_SRP_PASSWORD_HASH)
exit:
#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
    if (operation->alg == OBERON_PBKDF2_HMAC_ALG) {
        psa_driver_wrapper_hash_abort(&operation->hash_op);
    } else
#endif
    {
        psa_driver_wrapper_mac_abort(&operation->mac_op);
    }
    return status;
#endif
}

psa_status_t oberon_key_derivation_input_integer(
    oberon_key_derivation_operation_t *operation,
    psa_key_derivation_step_t step,
    uint64_t value)
{
    switch (step) {
#if defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128)
    case PSA_KEY_DERIVATION_INPUT_COST:
        operation->count = (uint32_t)value;
        return PSA_SUCCESS;
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC || PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 */
    default:
        (void)operation;
        (void)value;
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}

psa_status_t oberon_key_derivation_output_bytes(
    oberon_key_derivation_operation_t *operation,
    uint8_t *output, size_t output_length)
{
    psa_status_t status;
    size_t block_length = operation->block_length;
    size_t data_length = operation->data_length;
    size_t i, length;
    uint8_t u[PSA_HASH_MAX_SIZE];
    uint8_t idx;

    if (output_length == 0) return PSA_SUCCESS;

    if (data_length) {
        if (data_length >= output_length) {
            memcpy(output, operation->data + block_length - data_length, output_length);
            operation->data_length = (uint8_t)(data_length - output_length);
            return PSA_SUCCESS;
        } else {
            memcpy(output, operation->data + block_length - data_length, data_length);
            output += data_length;
            output_length -= data_length;
        }
    }

#ifdef PSA_NEED_OBERON_SP800_108_COUNTER_CMAC
    if (operation->alg == OBERON_SP800_108_COUNTER_ALG && operation->key_type == PSA_KEY_TYPE_AES) {
        // setup K0
        // key
        status = oberon_setup_mac(operation, operation->key, operation->key_length);
        if (status) goto exit;
        // label + context
        status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->info, operation->info_length);
        if (status) goto exit;
        // L
        status = oberon_mac_update_num(operation, operation->count);
        if (status) goto exit;
        status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, u, block_length, &length);
        if (status) goto exit;
    }
#endif

    // KDF expand
    for (;;) {
        switch (operation->alg) {

#ifdef PSA_NEED_OBERON_HKDF_EXTRACT
        case OBERON_HKDF_EXTRACT_ALG:
            // skip expand phase
            memcpy(operation->data, operation->key, block_length);
            break;
#endif /* PSA_NEED_OBERON_HKDF_EXTRACT */

#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXPAND)
        case OBERON_HKDF_ALG:
        case OBERON_HKDF_EXPAND_ALG:
            status = oberon_setup_mac(operation, operation->key, block_length); // prk
            if (status) goto exit;
            // T(i-1)
            if (operation->index > 1) {
                status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->data, block_length);
                if (status) goto exit;
            }
            // info
            status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->info, operation->info_length);
            if (status) goto exit;
            // i
            idx = (uint8_t)operation->index;
            status = psa_driver_wrapper_mac_update(&operation->mac_op, &idx, 1);
            if (status) goto exit;
            status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, operation->data, block_length, &length);
            if (status) goto exit;
            break;
#endif /* PSA_NEED_OBERON_HKDF || PSA_NEED_OBERON_HKDF_EXPAND */

#if defined(PSA_NEED_OBERON_TLS12_PRF) || defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS)
        case OBERON_TLS12_PRF_ALG:
        case OBERON_TLS12_PSK_TO_MS_ALG:
            // A(i) = HMAC(A(i-1))
            status = oberon_setup_mac(operation, operation->key, operation->key_length);
            if (status) goto exit;
            if (operation->index == 1) {
                status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->info, operation->info_length); // A(0)
            } else {
                status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->a, block_length); // A(i)
            }
            if (status) goto exit;
            status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, operation->a, block_length, &length);
            if (status) goto exit;
            // P_hash() = HMAC(A(i) || seed)
            status = oberon_setup_mac(operation, operation->key, operation->key_length);
            if (status) goto exit;
            status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->a, block_length); // A(i)
            if (status) goto exit;
            status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->info, operation->info_length); // seed
            if (status) goto exit;
            status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, operation->data, block_length, &length);
            if (status) goto exit;
            break;
#endif /* PSA_NEED_OBERON_TLS12_PRF || PSA_NEED_OBERON_TLS12_PSK_TO_MS */

#if defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128)
#ifdef PSA_NEED_OBERON_PBKDF2_HMAC
        case OBERON_PBKDF2_HMAC_ALG:
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC */
#ifdef PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128
        case OBERON_PBKDF2_CMAC_ALG:
#endif /* PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 */
            // U1 = mac(password, salt || int32be(i))
            status = oberon_setup_mac(operation, operation->key, operation->key_length);
            if (status) goto exit;
            status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->info, operation->salt_length);
            if (status) goto exit;
            status = oberon_mac_update_num(operation, operation->index);
            if (status) goto exit;
            status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, u, block_length, &length);
            if (status) goto exit;
            memcpy(operation->data, u, block_length);
            for (i = 1; i < operation->count; i++) {
                // Ui = mac(password, Ui-1)
                status = oberon_setup_mac(operation, operation->key, operation->key_length);
                if (status) goto exit;
                status = psa_driver_wrapper_mac_update(&operation->mac_op, u, block_length);
                if (status) goto exit;
                status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, u, block_length, &length);
                if (status) goto exit;
                // Ti ^= Ui
                oberon_xor(operation->data, operation->data, u, block_length);
            }
            break;
#endif /* PSA_NEED_OBERON_PBKDF2_HMAC || PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128 */

#ifdef PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS
        case OBERON_ECJPAKE_TO_PMS_ALG:
            if (output_length != PSA_TLS12_ECJPAKE_TO_PMS_DATA_SIZE) return PSA_ERROR_INVALID_ARGUMENT;
            return psa_driver_wrapper_hash_compute(PSA_ALG_SHA_256, operation->key, 32, output, output_length, &length);
#endif /* PSA_NEED_OBERON_TLS12_ECJPAKE_TO_PMS */

#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
        case OBERON_SRP_PASSWORD_HASH_ALG:
            status = psa_driver_wrapper_hash_finish(&operation->hash_op, output, output_length, &length);
            if (status != PSA_SUCCESS) psa_driver_wrapper_hash_abort(&operation->hash_op);
            if (output_length != length) return PSA_ERROR_INVALID_ARGUMENT;
            return status;
#endif

#if defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
        case OBERON_SP800_108_COUNTER_ALG:
            // key
            status = oberon_setup_mac(operation, operation->key, operation->key_length);
            if (status) goto exit;
            // i
            status = oberon_mac_update_num(operation, operation->index);
            if (status) goto exit;
            // label + context
            status = psa_driver_wrapper_mac_update(&operation->mac_op, operation->info, operation->info_length);
            if (status) goto exit;
            // L
            status = oberon_mac_update_num(operation, operation->count);
            if (status) goto exit;
#ifdef PSA_NEED_OBERON_SP800_108_COUNTER_CMAC
            if (operation->key_type == PSA_KEY_TYPE_AES) {
                // K0
                status = psa_driver_wrapper_mac_update(&operation->mac_op, u, block_length);
                if (status) goto exit;
            }
#endif
            // output
            status = psa_driver_wrapper_mac_sign_finish(&operation->mac_op, operation->data, block_length, &length);
            if (status) goto exit;
            break;
#endif /* PSA_NEED_OBERON_SP800_108_COUNTER_HMAC || PSA_NEED_OBERON_SP800_108_COUNTER_CMAC */

        default:
            (void)i;
            (void)u;
            (void)idx;
            (void)length;
            (void)status;
            return PSA_ERROR_BAD_STATE;
        }
        operation->index++;
        if (output_length > block_length) {
            memcpy(output, operation->data, block_length);
            output += block_length;
            output_length -= block_length;
        } else {
            memcpy(output, operation->data, output_length);
            operation->data_length = (uint8_t)(block_length - output_length);
            return PSA_SUCCESS;
        }
    }

#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXPAND) || \
    defined(PSA_NEED_OBERON_PBKDF2_HMAC) || defined(PSA_NEED_OBERON_PBKDF2_AES_CMAC_PRF_128) || \
    defined(PSA_NEED_OBERON_TLS12_PRF) || defined(PSA_NEED_OBERON_TLS12_PSK_TO_MS) || \
    defined(PSA_NEED_OBERON_SP800_108_COUNTER_HMAC) || defined(PSA_NEED_OBERON_SP800_108_COUNTER_CMAC)
exit:
    psa_driver_wrapper_mac_abort(&operation->mac_op);
    return status;
#endif
}

psa_status_t oberon_key_derivation_abort(
    oberon_key_derivation_operation_t *operation )
{
    switch (operation->alg) {
#if defined(PSA_NEED_OBERON_HKDF) || defined(PSA_NEED_OBERON_HKDF_EXTRACT)
    case OBERON_HKDF_ALG:
    case OBERON_HKDF_EXTRACT_ALG:
        return psa_driver_wrapper_mac_abort(&operation->mac_op);
#endif
#ifdef PSA_NEED_OBERON_SRP_PASSWORD_HASH
    case OBERON_SRP_PASSWORD_HASH_ALG:
        return psa_driver_wrapper_hash_abort(&operation->hash_op);
#endif
    default:
        return PSA_SUCCESS;
    }
}
