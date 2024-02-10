/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file is based on the Arm PSA Crypto Driver API.
// Different from the draft spec, the setup function has parameters, in order to
// enable an implementation without memory allocation in the driver.

#ifndef OBERON_SRP_H
#define OBERON_SRP_H

#include <psa/crypto_driver_common.h>
#include <psa/crypto_struct.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    psa_algorithm_t hash_alg;
    size_t          hash_len;
    uint8_t         password[384];
    uint8_t         ab[32];
    uint8_t         A[384];
    uint8_t         B[384];
    uint8_t         m1[PSA_HASH_MAX_SIZE];
    uint8_t         m2[PSA_HASH_MAX_SIZE];
    uint8_t         k[PSA_HASH_MAX_SIZE];
    uint8_t         user[256];
    size_t          user_len;
    uint8_t         salt[64];
    size_t          salt_len;
    psa_pake_role_t role;
} oberon_srp_operation_t;


psa_status_t oberon_srp_setup(
    oberon_srp_operation_t *operation,
    const psa_pake_cipher_suite_t *cipher_suite,
    const uint8_t *password, size_t password_length,
    const uint8_t *user_id, size_t user_id_length,
    const uint8_t *peer_id, size_t peer_id_length,
    psa_pake_role_t role);

psa_status_t oberon_srp_output(
    oberon_srp_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_srp_input(
    oberon_srp_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length);

psa_status_t oberon_srp_get_implicit_key(
    oberon_srp_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_srp_abort(
    oberon_srp_operation_t *operation);


#ifdef __cplusplus
}
#endif

#endif
