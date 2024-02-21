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
#include "oberon_jpake.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"

#include "ocrypto_ecjpake_p256.h"

#define P256_KEY_SIZE    32
#define P256_POINT_SIZE  64


// P256 generator in big-endian for hash calculation
static const uint8_t base[P256_POINT_SIZE] = {
    0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
    0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96,
    0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
    0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};


static psa_status_t oberon_update_hash_with_prefix(
    psa_hash_operation_t *hash_op,
    const uint8_t *data, size_t data_len,
    uint8_t type)
{
    psa_status_t status;
    uint8_t prefix[5];
    size_t hash_len = data_len;
    size_t prefix_len = 4;

    if (type) {
        prefix[4] = type; // add point type
        hash_len++;
        prefix_len++;
    }
    prefix[0] = 0;
    prefix[1] = 0;
    prefix[2] = (uint8_t)(hash_len >> 8);
    prefix[3] = (uint8_t)hash_len;
    status = psa_driver_wrapper_hash_update(hash_op, prefix, prefix_len);
    if (status != PSA_SUCCESS) return status;
    return psa_driver_wrapper_hash_update(hash_op, data, data_len);
}

static psa_status_t oberon_get_zkp_hash(
    psa_algorithm_t hash_alg,
    const uint8_t X[P256_POINT_SIZE],
    const uint8_t V[P256_POINT_SIZE],
    const uint8_t G[P256_POINT_SIZE],
    const uint8_t *id, size_t id_len,
    uint8_t *hash, size_t hash_size, size_t *hash_length)
{
    psa_status_t status;
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    status = psa_driver_wrapper_hash_setup(&hash_op, hash_alg);
    if (status != PSA_SUCCESS) goto exit;
    status = oberon_update_hash_with_prefix(&hash_op, G ? G : base, P256_POINT_SIZE, 0x04);
    if (status != PSA_SUCCESS) goto exit;
    status = oberon_update_hash_with_prefix(&hash_op, V, P256_POINT_SIZE, 0x04);
    if (status != PSA_SUCCESS) goto exit;
    status = oberon_update_hash_with_prefix(&hash_op, X, P256_POINT_SIZE, 0x04);
    if (status != PSA_SUCCESS) goto exit;
    status = oberon_update_hash_with_prefix(&hash_op, id, id_len, 0);
    if (status != PSA_SUCCESS) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, hash, hash_size, hash_length);
    if (status != PSA_SUCCESS) goto exit;
    return PSA_SUCCESS;
exit:
    psa_driver_wrapper_hash_abort(&hash_op);
    return status;
}

static psa_status_t oberon_write_key_share(
    oberon_jpake_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    int res;
    psa_status_t status;
    uint8_t generator[P256_POINT_SIZE];
    uint8_t v[P256_KEY_SIZE]; // ZKP secret key
    int idx = op->wr_idx; // key index
    uint8_t *gen = NULL;
    uint8_t h[PSA_HASH_MAX_SIZE];
    size_t h_len;

    if (idx == 2) { // second round
        // generator
        res = ocrypto_ecjpake_get_generator(generator, op->P[0], op->P[1], op->X[0]);
        gen = generator;
        // calculated secret key
        res |= ocrypto_ecjpake_process_shared_secret(op->x[2], op->x[1], op->secret);
        res |= ocrypto_ecjpake_get_public_key(op->X[2], gen, op->x[2]);
        if (res) return PSA_ERROR_INVALID_ARGUMENT; // we do not have a valid generator
    } else { // first round
        // random secret key
        do {
            status = psa_generate_random(op->x[idx], sizeof op->x[idx]);
            if (status != PSA_SUCCESS) return status;
        } while (ocrypto_ecjpake_get_public_key(op->X[idx], NULL, op->x[idx]));
    }

    // ZKP secret
    do {
        status = psa_generate_random(v, sizeof v);
        if (status != PSA_SUCCESS) return status;
    } while (ocrypto_ecjpake_get_public_key(op->V, gen, v));
    status = oberon_get_zkp_hash(op->hash_alg, op->X[idx], op->V, gen, op->user_id, op->user_id_length, h, sizeof h, &h_len);
    if (status != PSA_SUCCESS) return status;
    res = ocrypto_ecjpake_zkp_sign(op->r, op->x[idx], v, h, h_len);
    if (res) return PSA_ERROR_INVALID_ARGUMENT;

    if (sizeof op->X[idx] >= output_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    output[0] = 0x04;
    memcpy(&output[1], op->X[idx], sizeof op->X[idx]);
    *output_length = sizeof op->X[idx] + 1;

    return PSA_SUCCESS;
}

static psa_status_t oberon_write_zk_public(
    oberon_jpake_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    if (sizeof op->V >= output_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    output[0] = 0x04;
    memcpy(&output[1], op->V, sizeof op->V);
    *output_length = sizeof op->V + 1;

    return PSA_SUCCESS;
}

static psa_status_t oberon_write_zk_proof(
    oberon_jpake_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    if (sizeof op->r > output_size) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, op->r, sizeof op->r);
    *output_length = sizeof op->r;

    op->wr_idx++;
    return PSA_SUCCESS;
}

static psa_status_t oberon_read_key_share(
    oberon_jpake_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    int idx = op->rd_idx; // key index

    if (input_length != sizeof op->P[idx] + 1 || input[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
    memcpy(op->P[idx], &input[1], sizeof op->P[idx]);

    return PSA_SUCCESS;
}

static psa_status_t oberon_read_zk_public(
    oberon_jpake_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    if (input_length != sizeof op->V + 1 || input[0] != 0x04) return PSA_ERROR_INVALID_ARGUMENT;
    memcpy(op->V, &input[1], sizeof op->V);

    return PSA_SUCCESS;
}

static psa_status_t oberon_read_zk_proof(
    oberon_jpake_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    psa_status_t status;
    int res = 0;
    uint8_t generator[P256_POINT_SIZE];
    int idx = op->rd_idx; // key index
    uint8_t *rp = op->r;
    uint8_t *gen = NULL;
    uint8_t h[PSA_HASH_MAX_SIZE];
    size_t h_len;

    if (input_length > sizeof op->r) return PSA_ERROR_INVALID_SIGNATURE;
    if (input_length < sizeof op->r) {
        memset(rp, 0, sizeof op->r - input_length);
        rp += sizeof op->r - input_length;
    }
    memcpy(rp, input, input_length);

    if (idx == 2) { // second round
        res |= ocrypto_ecjpake_get_generator(generator, op->X[0], op->X[1], op->P[0]);
        gen = generator;
    }

    status = oberon_get_zkp_hash(op->hash_alg, op->P[idx], op->V, gen, op->peer_id, op->peer_id_length, h, sizeof h, &h_len);
    if (status != PSA_SUCCESS) return status;
    res |=  ocrypto_ecjpake_zkp_verify(gen, op->P[idx], op->V, op->r, h, h_len);
    if (res) return PSA_ERROR_INVALID_SIGNATURE;

    op->rd_idx++;
    return PSA_SUCCESS;
}


psa_status_t oberon_jpake_setup(
    oberon_jpake_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length,
    const psa_pake_cipher_suite_t *cipher_suite)
{
    (void)attributes;

    if (psa_pake_cs_get_primitive(cipher_suite) !=
            PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC, PSA_ECC_FAMILY_SECP_R1, 256)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    operation->hash_alg = PSA_ALG_GET_HASH(psa_pake_cs_get_algorithm(cipher_suite));
    if (operation->hash_alg != PSA_ALG_SHA_256) return PSA_ERROR_NOT_SUPPORTED;

    operation->rd_idx = 0;
    operation->wr_idx = 0;

    // store reduced password
    ocrypto_ecjpake_read_shared_secret(operation->secret, password, password_length);

    if (oberon_ct_compare_zero(operation->secret, sizeof operation->secret) == 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return PSA_SUCCESS;
}

psa_status_t oberon_jpake_set_user(
    oberon_jpake_operation_t *operation,
    const uint8_t *user_id, size_t user_id_len)
{
    if (user_id_len > sizeof operation->user_id) return PSA_ERROR_NOT_SUPPORTED;
    memcpy(operation->user_id, user_id, user_id_len);
    operation->user_id_length = (uint8_t)user_id_len;

    return PSA_SUCCESS;
}

psa_status_t oberon_jpake_set_peer(
    oberon_jpake_operation_t *operation,
    const uint8_t *peer_id, size_t peer_id_len)
{
    if (peer_id_len == operation->user_id_length) {
        if (memcmp(peer_id, operation->user_id, peer_id_len) == 0) {
            // user and peer ids must not be equal
            return PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    if (peer_id_len > sizeof operation->peer_id) return PSA_ERROR_NOT_SUPPORTED;
    memcpy(operation->peer_id, peer_id, peer_id_len);
    operation->peer_id_length = (uint8_t)peer_id_len;

    return PSA_SUCCESS;
}

psa_status_t oberon_jpake_output(
    oberon_jpake_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    switch (step) {
    case PSA_PAKE_STEP_KEY_SHARE:
        return oberon_write_key_share(
            operation,
            output, output_size, output_length);
    case PSA_PAKE_STEP_ZK_PUBLIC:
        return oberon_write_zk_public(
            operation,
            output, output_size, output_length);
    case PSA_PAKE_STEP_ZK_PROOF:
        return oberon_write_zk_proof(
            operation,
            output, output_size, output_length);
    default:
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}

psa_status_t oberon_jpake_input(
    oberon_jpake_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length)
{
    switch (step) {
    case PSA_PAKE_STEP_KEY_SHARE:
        return oberon_read_key_share(
            operation,
            input, input_length);
    case PSA_PAKE_STEP_ZK_PUBLIC:
        return oberon_read_zk_public(
            operation,
            input, input_length);
    case PSA_PAKE_STEP_ZK_PROOF:
        return oberon_read_zk_proof(
            operation,
            input, input_length);
    default:
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}

psa_status_t oberon_jpake_get_shared_key(
    oberon_jpake_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    int res;

    if (output_size < 65) return PSA_ERROR_BUFFER_TOO_SMALL;
    output[0] = 0x04; // curve point

    // get PMSK
    res = ocrypto_ecjpake_get_premaster_secret_key(&output[1],
        operation->P[2], operation->P[1],
        operation->x[2], operation->x[1]);
    if (res) return PSA_ERROR_INVALID_ARGUMENT;

    *output_length = 65;
    return PSA_SUCCESS;
}

psa_status_t oberon_jpake_abort(
    oberon_jpake_operation_t *operation)
{
    (void)operation;
    return PSA_SUCCESS;
}
