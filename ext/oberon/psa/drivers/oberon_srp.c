/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <string.h>

#include "psa/crypto.h"
#include "oberon_srp.h"
#include "oberon_helpers.h"
#include "psa_crypto_driver_wrappers.h"

#include "ocrypto_srp.h"

#define SRP_KEY_SIZE    (256/8)
#define SRP_FIELD_SIZE  (3072/8)


static const uint8_t oberon_P3072[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
    0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
    0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
    0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
    0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
    0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D, 0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
    0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
    0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
    0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
    0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
    0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33, 0x17, 0x0D, 0x04, 0x50, 0x7A, 0x33,
    0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64, 0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB, 0xEF, 0x0A,
    0x8A, 0xEA, 0x71, 0x57, 0x5D, 0x06, 0x0C, 0x7D, 0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7,
    0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0, 0x4A, 0x25, 0x61, 0x9D,
    0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B, 0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64,
    0xD8, 0x76, 0x02, 0x73, 0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F, 0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C,
    0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C, 0x77, 0x09, 0x88, 0xC0, 0xBA, 0xD9, 0x46, 0xE2,
    0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31, 0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E,
    0x4B, 0x82, 0xD1, 0x20, 0xA9, 0x3A, 0xD2, 0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t oberon_G3072[] = {5};


// hash of number with removed leading zeroes
static psa_status_t oberon_srp_hash_add_stripped(psa_hash_operation_t *op, const uint8_t *data, uint32_t dlen)
{
    while(dlen > 0 && *data == 0) { // skip leading zero bytes
        data++;
        dlen--;
    }
    return psa_driver_wrapper_hash_update(op, data, dlen);
}

static psa_status_t oberon_get_multiplier(oberon_srp_operation_t *op, psa_hash_operation_t *hash_op, uint8_t *k)
{
    psa_status_t status;
    size_t length;

    // k = H(p | pad(g))
    memset(k, 0, SRP_FIELD_SIZE);
    status = psa_driver_wrapper_hash_setup(hash_op, op->hash_alg);
    if (status) return status;
    status = psa_driver_wrapper_hash_update(hash_op, oberon_P3072, sizeof oberon_P3072);
    if (status) return status;
    status = psa_driver_wrapper_hash_update(hash_op, k, SRP_FIELD_SIZE - sizeof oberon_G3072);
    if (status) return status;
    status = psa_driver_wrapper_hash_update(hash_op, oberon_G3072, sizeof oberon_G3072);
    if (status) return status;
    return psa_driver_wrapper_hash_finish(hash_op, k + SRP_FIELD_SIZE - op->hash_len, op->hash_len, &length);
}

static psa_status_t oberon_get_proof(oberon_srp_operation_t *op)
{
    psa_status_t status;
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    uint8_t s[SRP_FIELD_SIZE];
    size_t hash_len;
    int res;

    // u = H(pad(A) | pad(B));
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->A, SRP_FIELD_SIZE);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->B, SRP_FIELD_SIZE);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->m1, sizeof op->m1, &hash_len);
    if (status) goto exit;

    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        // k = 0 | H(p | pad(g))
        status = oberon_get_multiplier(op, &hash_op, s);
        if (status) goto exit;
        // X = B - (k * g^pw), S = X^a * X^u^pw
        res = ocrypto_srp_client_premaster_secret(s, op->ab, op->B, s, op->m1, op->password, hash_len);
    } else {
        // S = (A * v^u) ^ b
        res = ocrypto_srp_server_premaster_secret(s, op->A, op->ab, op->m1, hash_len, op->password);
    }
    if (res) return PSA_ERROR_INVALID_ARGUMENT;

    // session key k = H(s)
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = oberon_srp_hash_add_stripped(&hash_op, s, SRP_FIELD_SIZE);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->k, sizeof op->k, &hash_len);
    if (status) goto exit;

    // H(p)
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, oberon_P3072, sizeof oberon_P3072);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->m1, sizeof op->m1, &hash_len);
    if (status) goto exit;

    // H(g)
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, oberon_G3072, sizeof oberon_G3072);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->m2, sizeof op->m2, &hash_len);
    if (status) goto exit;

    // H(p) ^ H(g)
    oberon_xor(op->m2, op->m2, op->m1, hash_len);

    // H(user)
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->user, op->user_len);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->m1, sizeof op->m1, &hash_len);
    if (status) goto exit;

    // m1 = H(H(p) ^ H(g) | H(user) | salt | A | B | k)
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->m2, hash_len); // H(p) ^ H(g)
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->m1, hash_len); // H(user)
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->salt, op->salt_len);
    if (status) goto exit;
    status = oberon_srp_hash_add_stripped(&hash_op, op->A, SRP_FIELD_SIZE);
    if (status) goto exit;
    status = oberon_srp_hash_add_stripped(&hash_op, op->B, SRP_FIELD_SIZE);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->k, hash_len);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->m1, sizeof op->m1, &hash_len);
    if (status) goto exit;

    // m2 = H(A | m1 | k)
    status = psa_driver_wrapper_hash_setup(&hash_op, op->hash_alg);
    if (status) goto exit;
    status = oberon_srp_hash_add_stripped(&hash_op, op->A, SRP_FIELD_SIZE);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->m1, hash_len);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_update(&hash_op, op->k, hash_len);
    if (status) goto exit;
    status = psa_driver_wrapper_hash_finish(&hash_op, op->m2, sizeof op->m2, &hash_len);
    if (status) goto exit;

    return PSA_SUCCESS;
exit:
    psa_hash_abort(&hash_op);
    return status;
}


static psa_status_t oberon_write_key_share(
    oberon_srp_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_status_t status;
    psa_hash_operation_t hash_op = PSA_HASH_OPERATION_INIT;
    uint8_t k[SRP_FIELD_SIZE];

    // random secret key
    status = psa_generate_random(op->ab, sizeof op->ab);
    if (status != PSA_SUCCESS) return status;

    if (output_size < SRP_FIELD_SIZE) return PSA_ERROR_BUFFER_TOO_SMALL;
    *output_length = SRP_FIELD_SIZE;
    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        // A = g^a
        ocrypto_srp_client_public_key(op->A, op->ab, sizeof op->ab);
        memcpy(output, op->A, SRP_FIELD_SIZE);
    } else {
        // k = H(p | g)
        status = oberon_get_multiplier(op, &hash_op, k);
        if (status) return status;
        // B = k*v + g^b
        ocrypto_srp_server_public_key(op->B, op->ab, k, op->password);
        memcpy(output, op->B, SRP_FIELD_SIZE);
    }

    return PSA_SUCCESS;
}

static psa_status_t oberon_read_key_share(
    oberon_srp_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    if (input_length != SRP_FIELD_SIZE) return PSA_ERROR_INVALID_ARGUMENT;
    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        memcpy(op->B, input, SRP_FIELD_SIZE);
    } else {
        memcpy(op->A, input, SRP_FIELD_SIZE);
    }

    return PSA_SUCCESS;
}

static psa_status_t oberon_write_confirm(
    oberon_srp_operation_t *op,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    psa_status_t status;

    if (output_size < op->hash_len) return PSA_ERROR_BUFFER_TOO_SMALL;
    if (op->role == PSA_PAKE_ROLE_CLIENT) {
        status = oberon_get_proof(op);
        if (status) return status;
        memcpy(output, op->m1, op->hash_len);
    } else {
        memcpy(output, op->m2, op->hash_len);
    }
    *output_length = op->hash_len;

    return PSA_SUCCESS;
}

static psa_status_t oberon_read_confirm(
    oberon_srp_operation_t *op,
    const uint8_t *input, size_t input_length)
{
    psa_status_t status;

    if (input_length != op->hash_len) return PSA_ERROR_INVALID_SIGNATURE;
    if (op->role == PSA_PAKE_ROLE_SERVER) {
        status = oberon_get_proof(op);
        if (status) return status;
        if (oberon_ct_compare(input, op->m1, op->hash_len)) return PSA_ERROR_INVALID_SIGNATURE;
    } else {
        if (oberon_ct_compare(input, op->m2, op->hash_len)) return PSA_ERROR_INVALID_SIGNATURE;
    }

    return PSA_SUCCESS;
}


psa_status_t oberon_srp_setup(
    oberon_srp_operation_t *operation,
    const psa_pake_cipher_suite_t *cipher_suite)
{
    if (cipher_suite->algorithm != PSA_ALG_SRP_6 ||
        cipher_suite->type != PSA_PAKE_PRIMITIVE_TYPE_DH ||
        cipher_suite->family != PSA_DH_FAMILY_RFC3526 ||
        cipher_suite->bits != 3072) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    operation->hash_alg = cipher_suite->hash;
    operation->hash_len = PSA_HASH_LENGTH(cipher_suite->hash);
    return PSA_SUCCESS;
}

psa_status_t oberon_srp_set_role(
    oberon_srp_operation_t *operation,
    psa_pake_role_t role)
{
    if (role != PSA_PAKE_ROLE_CLIENT && role != PSA_PAKE_ROLE_SERVER) {
        return PSA_ERROR_NOT_SUPPORTED;
    }
    operation->role = role;
    return PSA_SUCCESS;
}

psa_status_t oberon_srp_set_user(
    oberon_srp_operation_t *operation,
    const uint8_t *user_id, size_t user_id_len)
{
    if (user_id_len > sizeof operation->user) return PSA_ERROR_NOT_SUPPORTED;
    memcpy(operation->user, user_id, user_id_len);
    operation->user_len = user_id_len;
    return PSA_SUCCESS;
}

psa_status_t oberon_srp_set_password_key(
    oberon_srp_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length)
{
    if (operation->role == PSA_PAKE_ROLE_CLIENT) {
        // password hash
        if (password_length != operation->hash_len) return PSA_ERROR_INVALID_ARGUMENT;
        memcpy(operation->password, password, operation->hash_len);
    } else { /* role == PSA_PAKE_ROLE_SERVER */
        // password verifier
        if (password_length != SRP_FIELD_SIZE) return PSA_ERROR_INVALID_ARGUMENT;
        memcpy(operation->password, password, SRP_FIELD_SIZE);
    }

    (void)attributes;
    return PSA_SUCCESS;
}

psa_status_t oberon_srp_output(
    oberon_srp_operation_t *operation,
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

psa_status_t oberon_srp_input(
    oberon_srp_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length)
{
    switch (step) {
    case PSA_PAKE_STEP_SALT:
        if (input_length > sizeof operation->salt) return PSA_ERROR_NOT_SUPPORTED;
        memcpy(operation->salt, input, input_length);
        operation->salt_len = input_length;
        return PSA_SUCCESS;
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

psa_status_t oberon_srp_get_implicit_key(
    oberon_srp_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length)
{
    if (output_size < operation->hash_len) return PSA_ERROR_BUFFER_TOO_SMALL;
    memcpy(output, operation->k, operation->hash_len);
    *output_length = operation->hash_len;
    return PSA_SUCCESS;
}

psa_status_t oberon_srp_abort(
    oberon_srp_operation_t *operation)
{
    (void)operation;
    return PSA_SUCCESS;
}
