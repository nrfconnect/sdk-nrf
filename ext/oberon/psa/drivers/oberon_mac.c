/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_mac.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"


#define AES_BLOCK_LEN PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)


#ifdef PSA_NEED_OBERON_HMAC
static psa_status_t oberon_hmac_setup(
    oberon_hmac_operation_t *operation,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg)
{
    psa_status_t status;
    size_t i, length;
    psa_algorithm_t hash_alg = PSA_ALG_HMAC_GET_HASH(alg);
    size_t hash_len = PSA_HASH_LENGTH(hash_alg);
    size_t block_size = PSA_HASH_BLOCK_LENGTH(alg);

    if (key_length > block_size) {
        // replace key by H(key) stored in k
        status = psa_driver_wrapper_hash_setup(&operation->hash_op, hash_alg);
        if (status) return status;
        status = psa_driver_wrapper_hash_update(&operation->hash_op, key, key_length);
        if (status) return status;
        status = psa_driver_wrapper_hash_finish(&operation->hash_op, operation->k, hash_len, &length);
        if (status) return status;
        key = operation->k;
        key_length = hash_len;
    }

    status = psa_driver_wrapper_hash_setup(&operation->hash_op, hash_alg);
    if (status) return status;

    // k = key ^ ipad
    for (i = 0; i < key_length; i++) operation->k[i] = (uint8_t)(key[i] ^ 0x36);
    for (; i < block_size; i++) operation->k[i] = 0x36;
    status =  psa_driver_wrapper_hash_update(&operation->hash_op, operation->k, block_size); // key ^ ipad
    if (status) return status;

    operation->hash_alg = PSA_ALG_HMAC_GET_HASH(alg);
    operation->block_size = (uint8_t)block_size;
    return PSA_SUCCESS;
}

static psa_status_t oberon_hmac_update(
    oberon_hmac_operation_t *operation,
    const uint8_t *input, size_t input_len)
{
    return psa_driver_wrapper_hash_update(&operation->hash_op, input, input_len);
}

static psa_status_t oberon_hmac_finish(
    oberon_hmac_operation_t *operation,
    uint8_t *mac, size_t mac_size, size_t *mac_length)
{
    psa_status_t status;
    size_t i, hash_length;

    // H(K ^ ipad, in, num)
    status = psa_driver_wrapper_hash_finish(&operation->hash_op, operation->hash, sizeof operation->hash, &hash_length);
    if (status) goto exit;

    // k = key ^ opad = (key ^ ipad) ^ (ipad ^ opad) = k ^ (ipad ^ opad)
    for (i = 0; i < operation->block_size; i++) operation->k[i] = (uint8_t)(operation->k[i] ^ (0x36 ^ 0x5c));

    status = psa_driver_wrapper_hash_setup(&operation->hash_op, operation->hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&operation->hash_op, operation->k, operation->block_size);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&operation->hash_op, operation->hash, hash_length);
    if (status) goto exit;
    if (mac_size == hash_length) {
        return psa_driver_wrapper_hash_finish(&operation->hash_op, mac, mac_size, mac_length);
    } else { // truncated mac
        status = psa_driver_wrapper_hash_finish(&operation->hash_op, operation->hash, sizeof operation->hash, &hash_length);
        if (status) goto exit;
        memcpy(mac, operation->hash, mac_size);
        *mac_length = mac_size;
    }

exit:
    psa_driver_wrapper_hash_abort(&operation->hash_op);
    return status;
}

static psa_status_t oberon_hmac_compute(
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    uint8_t *mac, size_t mac_size, size_t *mac_length)
{
    oberon_hmac_operation_t operation = {{0}, 0, {0}, {0}, 0};
    psa_status_t status;

    status = oberon_hmac_setup(&operation, key, key_length, alg);
    if (status) goto exit;
    status = oberon_hmac_update(&operation, input, input_length);
    if (status) goto exit;
    return oberon_hmac_finish(&operation, mac, mac_size, mac_length);

exit:
    psa_driver_wrapper_hash_abort(&operation.hash_op);
    return status;
}
#endif /* PSA_NEED_OBERON_HMAC */


#ifdef PSA_NEED_OBERON_CMAC
// GF(2^128) multiply by x
// big-endian bit encoding (b127 in bit 7 of first byte, b0 in bit 0 of last byte)
static void gf128_multiply_x(uint8_t r[AES_BLOCK_LEN])
{
    int i;
    uint32_t c;

    c = r[0] >> 7; // msb
    c = c * 0x87;  // field reduction
    for (i = 15; i >= 0; i--) {
        c = ((uint32_t)r[i] << 1) ^ c;
        r[i] = (uint8_t)c;
        c >>= 8;
    }
}

static psa_status_t oberon_cmac_setup(
    oberon_cmac_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length)
{
    psa_status_t status;
    size_t length;

    operation->length = 0;

    status = psa_driver_wrapper_cipher_encrypt_setup(
        &operation->aes_op,
        attributes, key, key_length,
        PSA_ALG_ECB_NO_PADDING);
    if (status) goto exit;

    // subkey generation
    status = psa_driver_wrapper_cipher_update(
        &operation->aes_op,
        operation->block, AES_BLOCK_LEN,
        operation->k, AES_BLOCK_LEN, &length);
    if (status) goto exit;

    gf128_multiply_x(operation->k); // get k1

    return PSA_SUCCESS;

exit:
    psa_driver_wrapper_cipher_abort(&operation->aes_op);
    return status;
}

static psa_status_t oberon_cmac_update(
    oberon_cmac_operation_t *operation,
    const uint8_t *input, size_t input_length)
{
    psa_status_t status;
    size_t length, free;

    if (!input_length) return PSA_SUCCESS;

    if (operation->length) {
        free = AES_BLOCK_LEN - operation->length;
        if (input_length <= free) {
            oberon_xor(
                operation->block + operation->length,
                operation->block + operation->length,
                input, input_length);
            operation->length += input_length;
            return PSA_SUCCESS;
        } else {
            if (free) {
                oberon_xor(
                    operation->block + operation->length,
                    operation->block + operation->length,
                    input, free);
                input += free;
                input_length -= free;
            }
            status = psa_driver_wrapper_cipher_update(
                &operation->aes_op,
                operation->block, AES_BLOCK_LEN,
                operation->block, AES_BLOCK_LEN, &length);
            if (status) goto exit;
        }
    }

    while (input_length > AES_BLOCK_LEN) {
        oberon_xor(
            operation->block,
            operation->block,
            input, AES_BLOCK_LEN);
        status = psa_driver_wrapper_cipher_update(
            &operation->aes_op,
            operation->block, AES_BLOCK_LEN,
            operation->block, AES_BLOCK_LEN, &length);
        if (status) goto exit;
        input += AES_BLOCK_LEN;
        input_length -= AES_BLOCK_LEN;
    }
    /* 0 < msg_len <= AES_BLOCK_LEN */

    oberon_xor(
        operation->block,
        operation->block,
        input, input_length);
    operation->length = input_length;
    return PSA_SUCCESS;

exit:
    psa_driver_wrapper_cipher_abort(&operation->aes_op);
    return status;
}

static psa_status_t oberon_cmac_finish(
    oberon_cmac_operation_t *operation,
    uint8_t *mac, size_t mac_size, size_t *mac_length)
{
    psa_status_t status;
    size_t length;

    // handle last block
    if (operation->length == AES_BLOCK_LEN) {
        // full block
        oberon_xor(operation->block, operation->block, operation->k, AES_BLOCK_LEN); // use k1
    } else {
        // partial or empty block
        operation->block[operation->length] ^= 0x80; // padding pattern
        gf128_multiply_x(operation->k);   // k1 -> k2
        oberon_xor(operation->block, operation->block, operation->k, AES_BLOCK_LEN); // use k2
    }

    status = psa_driver_wrapper_cipher_update(
        &operation->aes_op,
        operation->block, AES_BLOCK_LEN,
        operation->block, AES_BLOCK_LEN, &length);
    if (status) goto exit;

    memcpy(mac, operation->block, mac_size);
    *mac_length = mac_size;

exit:
    psa_driver_wrapper_cipher_abort(&operation->aes_op);
    return status;
}

static psa_status_t oberon_cmac_compute(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    const uint8_t *input, size_t input_length,
    uint8_t *mac, size_t mac_size, size_t *mac_length)
{
    oberon_cmac_operation_t operation = {{0}, {0}, {0}, 0};
    psa_status_t status;

    status = oberon_cmac_setup(&operation, attributes, key, key_length);
    if (status) goto exit;
    status = oberon_cmac_update(&operation, input, input_length);
    if (status) goto exit;
    return oberon_cmac_finish(&operation, mac, mac_size, mac_length);

exit:
    psa_driver_wrapper_cipher_abort(&operation.aes_op);
    return status;
}
#endif /* PSA_NEED_OBERON_CMAC */


psa_status_t oberon_mac_sign_setup(
    oberon_mac_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg )
{
#ifdef PSA_NEED_OBERON_HMAC
    if (PSA_ALG_IS_HMAC(alg)) {
        memset(&operation->hmac, 0, sizeof operation->hmac);
        operation->alg = OBERON_HMAC_ALG;
        return oberon_hmac_setup(
            &operation->hmac,
            key, key_length,
            alg);
    } else
#endif /* PSA_NEED_OBERON_HMAC */
#ifdef PSA_NEED_OBERON_CMAC
    if (PSA_ALG_FULL_LENGTH_MAC(alg) == PSA_ALG_CMAC) {
        if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) return PSA_ERROR_NOT_SUPPORTED;
        if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        memset(&operation->cmac, 0, sizeof operation->cmac);
        operation->alg = OBERON_CMAC_ALG;
        return oberon_cmac_setup(
            &operation->cmac,
            attributes, key, key_length);
    } else
#endif /* PSA_NEED_OBERON_CMAC */
    {
        (void)operation;
        (void)attributes;
        (void)key;
        (void)key_length;
        (void)alg;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}

psa_status_t oberon_mac_verify_setup(
    oberon_mac_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg )
{
    return oberon_mac_sign_setup(operation, attributes, key, key_length, alg);
}

psa_status_t oberon_mac_update(
    oberon_mac_operation_t *operation,
    const uint8_t *input, size_t input_length )
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_HMAC
    case OBERON_HMAC_ALG:
        return oberon_hmac_update(
            &operation->hmac,
            input, input_length);
#endif /* PSA_NEED_OBERON_HMAC */
#ifdef PSA_NEED_OBERON_CMAC
    case OBERON_CMAC_ALG:
        return oberon_cmac_update(
            &operation->cmac,
            input, input_length);
#endif /* PSA_NEED_OBERON_CMAC */
    default:
        (void)input;
        (void)input_length;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_mac_sign_finish(
    oberon_mac_operation_t *operation,
    uint8_t *mac, size_t mac_size, size_t *mac_length )
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_HMAC
    case OBERON_HMAC_ALG:
        return oberon_hmac_finish(
            &operation->hmac,
            mac, mac_size, mac_length);
#endif /* PSA_NEED_OBERON_HMAC */
#ifdef PSA_NEED_OBERON_CMAC
    case OBERON_CMAC_ALG:
        return oberon_cmac_finish(
            &operation->cmac,
            mac, mac_size, mac_length);
#endif /* PSA_NEED_OBERON_CMAC */
    default:
        (void)mac;
        (void)mac_size;
        (void)mac_length;
        return PSA_ERROR_BAD_STATE;
    }
}

psa_status_t oberon_mac_verify_finish(
    oberon_mac_operation_t *operation,
    const uint8_t *mac, size_t mac_length)
{
    psa_status_t status;
    uint8_t temp_mac[PSA_HASH_MAX_SIZE];
    size_t mac_len;

    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_HMAC
    case OBERON_HMAC_ALG:
        status = oberon_hmac_finish(
            &operation->hmac,
            temp_mac, mac_length, &mac_len);
        break;
#endif /* PSA_NEED_OBERON_HMAC */
#ifdef PSA_NEED_OBERON_CMAC
    case OBERON_CMAC_ALG:
        status = oberon_cmac_finish(
            &operation->cmac,
            temp_mac, mac_length, &mac_len);
        break;
#endif /* PSA_NEED_OBERON_CMAC */
    default:
        (void)mac_len;
        return PSA_ERROR_BAD_STATE;
    }
    if (status != PSA_SUCCESS) return status;
    if (oberon_ct_compare(mac, temp_mac, mac_length)) return PSA_ERROR_INVALID_SIGNATURE;
    return PSA_SUCCESS;
}

psa_status_t oberon_mac_abort(
    oberon_mac_operation_t *operation )
{
    switch (operation->alg) {
#ifdef PSA_NEED_OBERON_HMAC
    case OBERON_HMAC_ALG:
        psa_driver_wrapper_hash_abort(&operation->hmac.hash_op);
        break;
#endif /* PSA_NEED_OBERON_HMAC */
#ifdef PSA_NEED_OBERON_CMAC
    case OBERON_CMAC_ALG:
        psa_driver_wrapper_cipher_abort(&operation->cmac.aes_op);
        break;
#endif /* PSA_NEED_OBERON_CMAC */
    default:
        return PSA_SUCCESS;
    }
    memset(operation, 0, sizeof *operation);
    return PSA_SUCCESS;
}


psa_status_t oberon_mac_compute(
    const psa_key_attributes_t *attributes,
    const uint8_t *key, size_t key_length,
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    uint8_t *mac, size_t mac_size, size_t *mac_length )
{
#ifdef PSA_NEED_OBERON_HMAC
    if (PSA_ALG_IS_HMAC(alg)) {
        return oberon_hmac_compute(
            key, key_length,
            alg,
            input, input_length,
            mac, mac_size, mac_length);
    } else
#endif /* PSA_NEED_OBERON_HMAC */
#ifdef PSA_NEED_OBERON_CMAC
    if (PSA_ALG_FULL_LENGTH_MAC(alg) == PSA_ALG_CMAC) {
        if (psa_get_key_type(attributes) != PSA_KEY_TYPE_AES) return PSA_ERROR_NOT_SUPPORTED;
        if (key_length != 16 && key_length != 24 && key_length != 32) return PSA_ERROR_INVALID_ARGUMENT;
        return oberon_cmac_compute(
            attributes, key, key_length,
            input, input_length,
            mac, mac_size, mac_length);
    } else
#endif /* PSA_NEED_OBERON_CMAC */
    {
        (void)attributes;
        (void)key;
        (void)key_length;
        (void)alg;
        (void)input;
        (void)input_length;
        (void)mac;
        (void)mac_size;
        (void)mac_length;
        return PSA_ERROR_NOT_SUPPORTED;
    }
}
