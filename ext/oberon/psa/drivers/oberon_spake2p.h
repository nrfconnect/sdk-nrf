/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef OBERON_SPAKE2P_H
#define OBERON_SPAKE2P_H

#include <psa/crypto_driver_common.h>
#include <psa/crypto_struct.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    psa_hash_operation_t hash_op; // TT
    psa_algorithm_t hash_alg;
    size_t          hash_len;
    uint8_t         w0[32];
    uint8_t         w1[32];
    uint8_t         L[65];
    uint8_t         xy[32];
    uint8_t         XY[65];
    uint8_t         YX[65];
    uint8_t         shared[PSA_HASH_MAX_SIZE];
    uint8_t         KconfPV[PSA_HASH_MAX_SIZE];
    uint8_t         KconfVP[PSA_HASH_MAX_SIZE];
    const uint8_t   *MN;
    const uint8_t   *NM;
    psa_pake_role_t role;
} oberon_spake2p_operation_t;


psa_status_t oberon_spake2p_setup(
    oberon_spake2p_operation_t *operation,
    const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t oberon_spake2p_set_password_key(
    oberon_spake2p_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length);

psa_status_t oberon_spake2p_set_user(
    oberon_spake2p_operation_t *operation,
    const uint8_t *user_id, size_t user_id_len);

psa_status_t oberon_spake2p_set_peer(
    oberon_spake2p_operation_t *operation,
    const uint8_t *peer_id, size_t peer_id_len);

psa_status_t oberon_spake2p_set_role(
    oberon_spake2p_operation_t *operation,
    psa_pake_role_t role);

psa_status_t oberon_spake2p_output(
    oberon_spake2p_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_spake2p_input(
    oberon_spake2p_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length);

psa_status_t oberon_spake2p_get_implicit_key(
    oberon_spake2p_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_spake2p_abort(
    oberon_spake2p_operation_t *operation);


#ifdef __cplusplus
}
#endif

#endif
