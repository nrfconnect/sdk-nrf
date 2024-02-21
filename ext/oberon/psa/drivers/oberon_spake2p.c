/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file implements functions from the Arm PSA Crypto Driver API.
// Different from the draft spec, the setup function has parameters, in order to
// enable an implementation without memory allocation in the driver.

#include <string.h>

#include "psa/crypto.h"
#include "oberon_spake2p.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"

#include "ocrypto_ecdh_p256.h"
#include "ocrypto_spake2p_p256.h"


#define P256_KEY_SIZE    32
#define P256_POINT_SIZE  65


// predefined 'random elements'
static const uint8_t M[P256_POINT_SIZE] = {0x04,
    0x88, 0x6e, 0x2f, 0x97, 0xac, 0xe4, 0x6e, 0x55, 0xba, 0x9d, 0xd7, 0x24, 0x25, 0x79, 0xf2, 0x99,
    0x3b, 0x64, 0xe1, 0x6e, 0xf3, 0xdc, 0xab, 0x95, 0xaf, 0xd4, 0x97, 0x33, 0x3d, 0x8f, 0xa1, 0x2f,
    0x5f, 0xf3, 0x55, 0x16, 0x3e, 0x43, 0xce, 0x22, 0x4e, 0x0b, 0x0e, 0x65, 0xff, 0x02, 0xac, 0x8e,
    0x5c, 0x7b, 0xe0, 0x94, 0x19, 0xc7, 0x85, 0xe0, 0xca, 0x54, 0x7d, 0x55, 0xa1, 0x2e, 0x2d, 0x20};
static const uint8_t N[P256_POINT_SIZE] = {0x04,
    0xd8, 0xbb, 0xd6, 0xc6, 0x39, 0xc6, 0x29, 0x37, 0xb0, 0x4d, 0x99, 0x7f, 0x38, 0xc3, 0x77, 0x07,
    0x19, 0xc6, 0x29, 0xd7, 0x01, 0x4d, 0x49, 0xa2, 0x4b, 0x4f, 0x98, 0xba, 0xa1, 0x29, 0x2b, 0x49,
    0x07, 0xd6, 0x0a, 0xa6, 0xbf, 0xad, 0xe4, 0x50, 0x08, 0xa6, 0x36, 0x33, 0x7f, 0x51, 0x68, 0xc6,
    0x4d, 0x9b, 0xd3, 0x60, 0x34, 0x80, 0x8c, 0xd5, 0x64, 0x49, 0x0b, 0x1e, 0x65, 0x6e, 0xdb, 0xe7};


static psa_status_t oberon_update_hash_with_prefix(
    psa_hash_operation_t *hash_op,
    const uint8_t *data, size_t data_len)
{
    psa_status_t status;
    uint8_t len[8];

    memset(len, 0, sizeof len);
    len[0] = (uint8_t)data_len;
    len[1] = (uint8_t)(data_len >> 8);
    status = psa_driver_wrapper_hash_update(hash_op, len, sizeof len);
    if (status != PSA_SUCCESS) goto exit;
    status = psa_driver_wrapper_hash_update(hash_op, data, data_len);
    if (status != PSA_SUCCESS) goto exit;
    return PSA_SUCCESS;
exit:
    psa_driver_wrapper_hash_abort(hash_op);
    return status;
}

static psa_status_t oberon_update_ids(oberon_spake2p_operation_t *op)
{
    psa_status_t status;

    // add idProver to TT
    status = oberon_update_hash_with_prefix(&op->hash_op, op->prover, op->prover_len);
    if (status != PSA_SUCCESS) return status;
    // add idVerifier to TT
    status = oberon_update_hash_with_prefix(&op->hash_op, op->verifier, op->verifier_len);
    if (status != PSA_SUCCESS) return status;
    // add M to TT
    status = oberon_update_hash_with_prefix(&op->hash_op, M, sizeof M);
    if (status != PSA_SUCCESS) return status;
    // add N to TT
    return oberon_update_hash_with_prefix(&op->hash_op, N, sizeof N);
}

static psa_status_t oberon_write_key_share(
    oberon_spake2p_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    int res;
    psa_status_t status;
    const uint8_t *mn;

    // random secret key
    status = psa_generate_random(op->XY, 40);
    if (status != PSA_SUCCESS) return status;
    ocrypto_spake2p_p256_reduce(op->xy, op->XY, 40);

    mn = op->role == PSA_PAKE_ROLE_CLIENT ? M : N;
    res = ocrypto_spake2p_p256_get_key_share(op->XY, op->w0, op->xy, mn);
    if (res) return PSA_ERROR_INVALID_ARGUMENT;

    if (output_size < P256_POINT_SIZE) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, op->XY, P256_POINT_SIZE);
    *output_length = P256_POINT_SIZE;

    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        // add ids, M, and N to TT
        status = oberon_update_ids(op);
        if (status != PSA_SUCCESS) return status;
    }

    // add share to TT
    return oberon_update_hash_with_prefix(&op->hash_op, op->XY, P256_POINT_SIZE);
}

static psa_status_t oberon_read_key_share(
    oberon_spake2p_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    psa_status_t status;

    if (input_length != P256_POINT_SIZE || input[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
    memcpy(op->YX, input, P256_POINT_SIZE);

    if (op->role == PSA_PAKE_ROLE_SERVER) {
        // add ids, M, and N to TT
        status = oberon_update_ids(op);
        if (status != PSA_SUCCESS) return status;
    }

    // add share to TT
    return oberon_update_hash_with_prefix(&op->hash_op, op->YX, P256_POINT_SIZE);
}

static psa_status_t oberon_get_confirmation_keys(
    oberon_spake2p_operation_t *op,
    uint8_t *KconfP, uint8_t *KconfV)
{
    psa_status_t status;
    psa_algorithm_t hkdf_alg = PSA_ALG_HKDF(PSA_ALG_GET_HASH(op->alg));
    psa_key_derivation_operation_t kdf_op = PSA_KEY_DERIVATION_OPERATION_INIT;
    uint8_t Z[P256_POINT_SIZE];
    uint8_t V[P256_POINT_SIZE];
    size_t hash_len, conf_len = 0, shared_len = 0, mac_len = 0;

    // add Z, V, and w0 to TT
    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        ocrypto_spake2p_p256_get_ZV(Z, V, op->w0, &op->w1L[1], op->xy, op->YX, N, NULL);
    } else {
        ocrypto_spake2p_p256_get_ZV(Z, V, op->w0, NULL, op->xy, op->YX, M, op->w1L);
    }
    status = oberon_update_hash_with_prefix(&op->hash_op, Z, P256_POINT_SIZE);
    if (status) return status;
    status = oberon_update_hash_with_prefix(&op->hash_op, V, P256_POINT_SIZE);
    if (status) return status;
    status = oberon_update_hash_with_prefix(&op->hash_op, op->w0, P256_KEY_SIZE);
    if (status) return status;

    // get K_main
    status = psa_driver_wrapper_hash_finish(&op->hash_op, V, sizeof V, &hash_len);
    if (status) {
        psa_driver_wrapper_hash_abort(&op->hash_op);
        return status;
    }

    // get K_shared
#ifdef PSA_NEED_OBERON_SPAKE2P_MATTER
    if (op->alg == PSA_ALG_SPAKE2P_MATTER) {
        // Spake2+ draft version 2
        conf_len = hash_len >> 1;   // K_confirm is hash_len / 2
        shared_len = hash_len >> 1; // shared key size is hash_len / 2
        mac_len = hash_len;         // mac size is hash_len
        memcpy(op->shared, V + conf_len, shared_len);
    } else
#endif
    {
#if defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) || defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256)
        shared_len = hash_len;
#if defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) && defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256)
        if (PSA_ALG_IS_SPAKE2P_CMAC(op->alg)) {
#endif
#ifdef PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256
            mac_len = PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, 128, PSA_ALG_CMAC);
#endif
#if defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) && defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256)
        } else {
#endif
#ifdef PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256
            mac_len = hash_len;
#endif
#if defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) && defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256)
        }
#endif
        conf_len = mac_len;
        status = psa_driver_wrapper_key_derivation_setup(&kdf_op, hkdf_alg);
        if (status) goto exit;
        status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_INFO, (uint8_t *)"SharedKey", 9);
        if (status) goto exit;
        status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_SECRET, V, hash_len);
        if (status) goto exit;
        status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, op->shared, shared_len);
        if (status) goto exit;
        psa_key_derivation_abort(&kdf_op);
#endif /* PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256 || PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256 */
    }

    op->conf_len = (uint8_t)conf_len;
    op->shared_len = (uint8_t)shared_len;
    op->mac_len = (uint8_t)mac_len;

    // get K_confirmP & K_confirmV
    status = psa_driver_wrapper_key_derivation_setup(&kdf_op, hkdf_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_INFO, (uint8_t *)"ConfirmationKeys", 16);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_SECRET, V, conf_len);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, KconfP, conf_len);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, KconfV, conf_len);

exit:
    psa_driver_wrapper_key_derivation_abort(&kdf_op);
    memset(Z, 0, sizeof Z);
    memset(V, 0, sizeof V);
    return status;
}

static psa_status_t oberon_get_confirmation(
    oberon_spake2p_operation_t *op,
    const uint8_t *kconf,
    const uint8_t *share,
    uint8_t *conf)
{
    size_t length;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_algorithm_t mac_alg = 0;

#if defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256) && \
   (defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) || defined(PSA_NEED_OBERON_SPAKE2P_MATTER))
    if (PSA_ALG_IS_SPAKE2P_CMAC(op->alg)) {
#endif
#ifdef PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256
        mac_alg = PSA_ALG_CMAC;
        psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
#endif
#if defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256) && \
   (defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) || defined(PSA_NEED_OBERON_SPAKE2P_MATTER))
    } else {
#endif
#if defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) || defined(PSA_NEED_OBERON_SPAKE2P_MATTER)
        mac_alg = PSA_ALG_HMAC(PSA_ALG_GET_HASH(op->alg));
        psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
#endif
#if defined(PSA_NEED_OBERON_SPAKE2P_CMAC_SECP_R1_256) && \
   (defined(PSA_NEED_OBERON_SPAKE2P_HMAC_SECP_R1_256) || defined(PSA_NEED_OBERON_SPAKE2P_MATTER))
    }
#endif

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, mac_alg);

    return psa_driver_wrapper_mac_compute(
        &attributes, kconf, op->conf_len,
        mac_alg,
        share, P256_POINT_SIZE,
        conf, op->mac_len, &length);
}

static psa_status_t oberon_write_confirm(
    oberon_spake2p_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_status_t status;

    if (op->role == PSA_PAKE_ROLE_SERVER) {
        status = oberon_get_confirmation_keys(op, op->KconfVP, op->KconfPV);
        if (status) return status;
    }

    if (output_size < op->mac_len) return PSA_ERROR_BUFFER_TOO_SMALL;
    status = oberon_get_confirmation(op, op->KconfPV, op->YX, output);
    if (status) return status;
    *output_length = op->mac_len;

    return PSA_SUCCESS;
}

static psa_status_t oberon_read_confirm(
    oberon_spake2p_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    psa_status_t status;
    uint8_t conf[PSA_HASH_MAX_SIZE];

    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        status = oberon_get_confirmation_keys(op, op->KconfPV, op->KconfVP);
        if (status) return status;
    }

    status = oberon_get_confirmation(op, op->KconfVP, op->XY, conf);
    if (status) return status;

    if (input_length != op->mac_len) return PSA_ERROR_INVALID_SIGNATURE;
    if (oberon_ct_compare(input, conf, op->mac_len)) return PSA_ERROR_INVALID_SIGNATURE;

    return PSA_SUCCESS;
}


psa_status_t oberon_spake2p_setup(
    oberon_spake2p_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length,
    const psa_pake_cipher_suite_t *cipher_suite)
{
    (void)attributes;

    if (psa_pake_cs_get_primitive(cipher_suite) !=
            PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC, PSA_ECC_FAMILY_SECP_R1, 256) ||
        psa_pake_cs_get_key_confirmation(cipher_suite) != PSA_PAKE_CONFIRMED_KEY) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (password_length == 2 * P256_KEY_SIZE) {
        // password = w0:w1
        memcpy(operation->w0, password, P256_KEY_SIZE);
        password += P256_KEY_SIZE;
        operation->w1L[0] = 0; // w1L is 0x00:w1
        ocrypto_spake2p_p256_reduce(&operation->w1L[1], password, P256_KEY_SIZE);
    } else if (password_length == P256_KEY_SIZE + P256_POINT_SIZE) {
        // password = w0:L
        memcpy(operation->w0, password, P256_KEY_SIZE);
        password += P256_KEY_SIZE;
        memcpy(operation->w1L, password, P256_POINT_SIZE); // w1L is L = 0x04:x:y
    } else {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    // prepare TT calculation
    operation->alg = psa_pake_cs_get_algorithm(cipher_suite);
    return psa_driver_wrapper_hash_setup(&operation->hash_op, PSA_ALG_GET_HASH(operation->alg));
}

psa_status_t oberon_spake2p_set_role(
    oberon_spake2p_operation_t *operation,
    psa_pake_role_t role)
{
    if (role == PSA_PAKE_ROLE_CLIENT) {
        if (operation->w1L[0] == 0x04) return PSA_ERROR_INVALID_ARGUMENT;
    } else {
        if (operation->w1L[0] != 0x04) { // secret key -> public key
            operation->w1L[0] = 0x04;
            ocrypto_ecdh_p256_public_key(&operation->w1L[1], &operation->w1L[1]);
        }
    }
    operation->role = role;
    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_set_user(
    oberon_spake2p_operation_t *operation,
    const uint8_t *user_id, size_t user_id_len)
{
    if (operation->role == PSA_PAKE_ROLE_CLIENT) {
        // prover = user; verifier = peer
        if (user_id_len > sizeof operation->prover) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(operation->prover, user_id, user_id_len);
        operation->prover_len = (uint8_t)user_id_len;
    } else { /* role == PSA_PAKE_ROLE_SERVER */
        // prover = peer; verifier = user
        if (user_id_len > sizeof operation->verifier) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(operation->verifier, user_id, user_id_len);
        operation->verifier_len = (uint8_t)user_id_len;
    }
    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_set_peer(
    oberon_spake2p_operation_t *operation,
    const uint8_t *peer_id, size_t peer_id_len)
{
    if (operation->role == PSA_PAKE_ROLE_CLIENT) {
        // prover = user; verifier = peer
        if (peer_id_len > sizeof operation->verifier) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(operation->verifier, peer_id, peer_id_len);
        operation->verifier_len = (uint8_t)peer_id_len;
    } else { /* role == PSA_PAKE_ROLE_SERVER */
        // prover = peer; verifier = user
        if (peer_id_len > sizeof operation->prover) {
            return PSA_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(operation->prover, peer_id, peer_id_len);
        operation->prover_len = (uint8_t)peer_id_len;
    }
    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_set_context(
    oberon_spake2p_operation_t *operation,
    const uint8_t *context, size_t context_len)
{
    if (context_len == 0) return PSA_SUCCESS;

    // add context to TT
    return oberon_update_hash_with_prefix(
        &operation->hash_op,
        context, context_len);
}

psa_status_t oberon_spake2p_output(
    oberon_spake2p_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    switch (step) {
    case PSA_PAKE_STEP_KEY_SHARE:
        return oberon_write_key_share(
            operation,
            output, output_size, output_length);
    case PSA_PAKE_STEP_CONFIRM:
        return oberon_write_confirm(
            operation,
            output, output_size, output_length);
    default:
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}

psa_status_t oberon_spake2p_input(
    oberon_spake2p_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length)
{
    switch (step) {
    case PSA_PAKE_STEP_KEY_SHARE:
        return oberon_read_key_share(
            operation,
            input, input_length);
    case PSA_PAKE_STEP_CONFIRM:
        return oberon_read_confirm(
            operation,
            input, input_length);
    default:
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}

psa_status_t oberon_spake2p_get_shared_key(
    oberon_spake2p_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    size_t shared_len = operation->shared_len;
    if (output_size < shared_len) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, operation->shared, shared_len);
    *output_length = shared_len;
    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_abort(
    oberon_spake2p_operation_t *operation)
{
    return psa_driver_wrapper_hash_abort(&operation->hash_op);
}


// key management

psa_status_t oberon_derive_spake2p_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *input, size_t input_length,
    uint8_t *key, size_t key_size, size_t *key_length)
{
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE_SECP_R1_256
    case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (bits != 256) return PSA_ERROR_NOT_SUPPORTED;
        if (input_length != 80) return PSA_ERROR_INVALID_ARGUMENT;
        if (key_size < 64) return PSA_ERROR_BUFFER_TOO_SMALL;
        ocrypto_spake2p_p256_reduce(key, input, 40);           // w0s -> w0
        if (!oberon_ct_compare_zero(key, 32)) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_spake2p_p256_reduce(key + 32, input + 40, 40); // w1s -> w1
        if (!oberon_ct_compare_zero(key + 32, 32)) return PSA_ERROR_INVALID_ARGUMENT;
        *key_length = 64;
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE_SECP_R1_256 */

    default:
        (void)input;
        (void)input_length;
        (void)key;
        (void)key_size;
        (void)key_length;
        (void)bits;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_import_spake2p_key(
    const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length,
    uint8_t *key, size_t key_size, size_t *key_length,
    size_t *key_bits)
{
    int res;
    size_t bits = psa_get_key_bits(attributes);
    psa_key_type_t type = psa_get_key_type(attributes);

    switch (type) {
#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT_SECP_R1_256
    case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (data_length != 64) return PSA_ERROR_NOT_SUPPORTED;
        if (bits != 0 && (bits != 256)) return PSA_ERROR_INVALID_ARGUMENT;
        if (!oberon_ct_compare_zero(data, 32)) return PSA_ERROR_INVALID_ARGUMENT;
        res = ocrypto_ecdh_p256_secret_key_check(data);
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // out of range
        if (!oberon_ct_compare_zero(data + 32, 32)) return PSA_ERROR_INVALID_ARGUMENT;
        res = ocrypto_ecdh_p256_secret_key_check(data + 32);
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // out of range
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT_SECP_R1_256 */

#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_PUBLIC_KEY_SECP_R1_256
    case PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1):
        if (data_length != 32 + 65) return PSA_ERROR_NOT_SUPPORTED;
        if (bits != 0 && (bits != 256)) return PSA_ERROR_INVALID_ARGUMENT;
        if (!oberon_ct_compare_zero(data, 32)) return PSA_ERROR_INVALID_ARGUMENT;
        res = ocrypto_ecdh_p256_secret_key_check(data);
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // out of range
        if (data[32] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
        res = ocrypto_ecdh_p256_public_key_check(&data[33]);
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // point not on curve
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_PUBLIC_KEY_SECP_R1_256 */

    default:
        (void)res;
        (void)bits;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (key_size < data_length) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(key, data, data_length);
    *key_length = data_length;
    *key_bits = 256;
    return PSA_SUCCESS;
}

psa_status_t oberon_export_spake2p_public_key(
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
#ifdef PSA_NEED_OBERON_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT_SECP_R1_256
    case PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (bits != 256) return PSA_ERROR_NOT_SUPPORTED;
        if (key_length != 64) return PSA_ERROR_INVALID_ARGUMENT;
        if (data_size < 32 + 65) return PSA_ERROR_BUFFER_TOO_SMALL;
        memcpy(data, key, 32); // w0
        data[32] = 0x04;
        res = ocrypto_ecdh_p256_public_key(&data[33], &key[32]); // w1 -> L
        if (res) return PSA_ERROR_INVALID_ARGUMENT;
        *data_length = 32 + 65;
        break;
#endif /* PSA_NEED_OBERON_KEY_TYPE_ECC_KEY_PAIR_EXPORT_SECP_R1_256 */
    default:
        (void)res;
        (void)bits;
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_SUCCESS;
}
