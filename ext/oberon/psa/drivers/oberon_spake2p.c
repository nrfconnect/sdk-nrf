/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_spake2p.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"

#include "ocrypto_spake2p_p256.h"


// up to version 04 the K_main, K_confirmP, and K_confirmV values were calculated differently
//#define SPAKE2P_USE_VERSION_04


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

static psa_status_t oberon_update_ids(
    oberon_spake2p_operation_t *op)
{
    psa_status_t status;

    // add prover, verifier, M, and N to TT
    status = oberon_update_hash_with_prefix(&op->hash_op, op->prover, op->prover_len);
    if (status) return status;
    status = oberon_update_hash_with_prefix(&op->hash_op, op->verifier, op->verifier_len);
    if (status) return status;
    status = oberon_update_hash_with_prefix(&op->hash_op, M, sizeof M);
    if (status) return status;
    return oberon_update_hash_with_prefix(&op->hash_op, N, sizeof N);
}

static psa_status_t oberon_write_key_share(
    oberon_spake2p_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    int res;
    psa_status_t status;
    uint8_t xs[40];

    // random secret key
    status = psa_generate_random(xs, sizeof xs);
    if (status != PSA_SUCCESS) return status;
    ocrypto_spake2p_p256_reduce(op->xy, xs, sizeof xs);

    res = ocrypto_spake2p_p256_get_key_share(op->XY, op->w0, op->xy, op->MN);
    if (res) return PSA_ERROR_INVALID_ARGUMENT;

    if (output_size < P256_POINT_SIZE) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, op->XY, P256_POINT_SIZE);
    *output_length = P256_POINT_SIZE;

    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        oberon_update_ids(op);
    }

    // add share to TT
    return oberon_update_hash_with_prefix(&op->hash_op, op->XY, P256_POINT_SIZE);
}

static psa_status_t oberon_read_key_share(
    oberon_spake2p_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    if (input_length != P256_POINT_SIZE || input[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
    memcpy(op->YX, input, P256_POINT_SIZE);

    if (op->role != PSA_PAKE_ROLE_CLIENT) {
        oberon_update_ids(op);
    }

    // add share to TT
    return oberon_update_hash_with_prefix(&op->hash_op, op->YX, P256_POINT_SIZE);
}

static psa_status_t oberon_get_confirmation_keys(
    oberon_spake2p_operation_t *op,
    uint8_t *KconfP, uint8_t *KconfV)
{
    psa_status_t status;
    psa_key_derivation_operation_t kdf_op = PSA_KEY_DERIVATION_OPERATION_INIT;
    uint8_t Z[P256_POINT_SIZE];
    uint8_t V[P256_POINT_SIZE];
    size_t hash_len;

    // add Z, V, and w0 to TT
    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        ocrypto_spake2p_p256_get_ZV(Z, V, op->w0, op->w1, op->xy, op->YX, op->NM, NULL);
    } else {
        ocrypto_spake2p_p256_get_ZV(Z, V, op->w0, NULL, op->xy, op->YX, op->NM, op->L);
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
    op->hash_len = hash_len;

    // get K_shared
#ifdef SPAKE2P_USE_VERSION_04
    hash_len >>= 1; // K_confirm and confirm size is hash_len / 2
    memcpy(op->shared, data + hash_len, hash_len);
#else
    status = psa_driver_wrapper_key_derivation_setup(&kdf_op, PSA_ALG_HKDF(op->hash_alg));
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_INFO, (uint8_t *)"SharedKey", 9);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_SECRET, V, hash_len);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, op->shared, hash_len);
    if (status) goto exit;
    psa_key_derivation_abort(&kdf_op);
#endif

    // get K_confirmP & K_confirmV
    status = psa_driver_wrapper_key_derivation_setup(&kdf_op, PSA_ALG_HKDF(op->hash_alg));
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_INFO, (uint8_t *)"ConfirmationKeys", 16);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_input_bytes(&kdf_op, PSA_KEY_DERIVATION_INPUT_SECRET, V, hash_len);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, KconfP, hash_len);
    if (status) goto exit;
    status = psa_driver_wrapper_key_derivation_output_bytes(&kdf_op, KconfV, hash_len);

exit:
    psa_driver_wrapper_key_derivation_abort(&kdf_op);
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
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(op->hash_alg));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);

    return psa_driver_wrapper_mac_compute(
#ifdef SPAKE2P_USE_VERSION_04
        &attributes, kconf, op->hash_len >> 1,
#else
        &attributes, kconf, op->hash_len,
#endif
        PSA_ALG_HMAC(op->hash_alg),
        share, P256_POINT_SIZE,
        conf, op->hash_len, &length);
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

    if (output_size < op->hash_len) return PSA_ERROR_BUFFER_TOO_SMALL;
    status = oberon_get_confirmation(op, op->KconfPV, op->YX, output);
    if (status) return status;
    *output_length = op->hash_len;

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

    if (input_length != op->hash_len) return PSA_ERROR_INVALID_SIGNATURE;
    if (oberon_ct_compare(input, conf, op->hash_len)) return PSA_ERROR_INVALID_SIGNATURE;

    return PSA_SUCCESS;
}


psa_status_t oberon_spake2p_setup(
    oberon_spake2p_operation_t *operation,
    const psa_pake_cipher_suite_t *cipher_suite)
{
    if (cipher_suite->algorithm != PSA_ALG_SPAKE2P ||
        cipher_suite->type != PSA_PAKE_PRIMITIVE_TYPE_ECC ||
        cipher_suite->family != PSA_ECC_FAMILY_SECP_R1 ||
        cipher_suite->bits != 256) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    // prepare TT calculation
    operation->hash_alg = cipher_suite->hash;
    return psa_driver_wrapper_hash_setup(&operation->hash_op, cipher_suite->hash);
}

psa_status_t oberon_spake2p_set_role(
    oberon_spake2p_operation_t *operation,
    psa_pake_role_t role)
{
    if (role == PSA_PAKE_ROLE_CLIENT) {
        operation->MN = M;
        operation->NM = N;
    } else if (role == PSA_PAKE_ROLE_SERVER) {
        operation->MN = N;
        operation->NM = M;
    } else {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    operation->role = role;
    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_set_user(
    oberon_spake2p_operation_t *operation,
    const uint8_t *user_id, size_t user_id_len)
{
    if (operation->role == PSA_PAKE_ROLE_CLIENT) {
        // prover = user
        if (user_id_len > sizeof operation->prover) return PSA_ERROR_INSUFFICIENT_MEMORY;
        memcpy(operation->prover, user_id, user_id_len);
        operation->prover_len = (uint8_t)user_id_len;
    } else {
        // verifier = user
        if (user_id_len > sizeof operation->verifier) return PSA_ERROR_INSUFFICIENT_MEMORY;
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
        // verifier = peer
        if (peer_id_len > sizeof operation->verifier) return PSA_ERROR_INSUFFICIENT_MEMORY;
        memcpy(operation->verifier, peer_id, peer_id_len);
        operation->verifier_len = (uint8_t)peer_id_len;
    } else {
        // prover = peer
        if (peer_id_len > sizeof operation->prover) return PSA_ERROR_INSUFFICIENT_MEMORY;
        memcpy(operation->prover, peer_id, peer_id_len);
        operation->prover_len = (uint8_t)peer_id_len;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_set_password_key(
    oberon_spake2p_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length)
{
    int res;
    (void)attributes;

    if (operation->role == PSA_PAKE_ROLE_CLIENT) {
        // password = w0s:w1s
        if (password_length < 2 * P256_KEY_SIZE) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_spake2p_p256_reduce(operation->w0, password, password_length >> 1);
        password += password_length >> 1;
        ocrypto_spake2p_p256_reduce(operation->w1, password, password_length >> 1);
    } else { /* role == PSA_PAKE_ROLE_SERVER */
        // password = w0s:L
        if (password_length < P256_KEY_SIZE + P256_POINT_SIZE) return PSA_ERROR_INVALID_ARGUMENT;
        ocrypto_spake2p_p256_reduce(operation->w0, password, password_length - P256_POINT_SIZE);
        password += password_length - P256_POINT_SIZE;
        res = ocrypto_spake2p_p256_check_key(password);
        if (res) return PSA_ERROR_INVALID_ARGUMENT;
        memcpy(operation->L, password, P256_POINT_SIZE);
    }

    return PSA_SUCCESS;
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
    case PSA_PAKE_STEP_CONTEXT:
        // add context to TT
        return oberon_update_hash_with_prefix(&operation->hash_op, input, input_length);
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

psa_status_t oberon_spake2p_get_implicit_key(
    oberon_spake2p_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length)
{
#ifdef SPAKE2P_USE_VERSION_04
    if (output_size < operation->hash_len >> 1) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, operation->shared, operation->hash_len >> 1);
    *output_length = operation->hash_len >> 1;
#else
    if (output_size < operation->hash_len) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, operation->shared, operation->hash_len);
    *output_length = operation->hash_len;
#endif
    return PSA_SUCCESS;
}

psa_status_t oberon_spake2p_abort(
    oberon_spake2p_operation_t *operation)
{
    return psa_driver_wrapper_hash_abort(&operation->hash_op);
}
