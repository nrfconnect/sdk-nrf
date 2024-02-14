/*
 * Copyright (c) 2016 - 2024 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//
// This file is based on the Arm PSA Crypto Driver API.

#ifndef OBERON_PAKE_H
#define OBERON_PAKE_H

#include <psa/crypto_driver_common.h>

#ifdef PSA_NEED_OBERON_JPAKE
#include "oberon_jpake.h"
#endif
#ifdef PSA_NEED_OBERON_SPAKE2P
#include "oberon_spake2p.h"
#endif
#ifdef PSA_NEED_OBERON_SRP_6
#include "oberon_srp.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    psa_algorithm_t alg;
    union {
        unsigned dummy; /* Make sure this union is always non-empty */
#ifdef PSA_NEED_OBERON_JPAKE
        oberon_jpake_operation_t oberon_jpake_ctx;
#endif
#ifdef PSA_NEED_OBERON_SPAKE2P
        oberon_spake2p_operation_t oberon_spake2p_ctx;
#endif
#ifdef PSA_NEED_OBERON_SRP_6
        oberon_srp_operation_t oberon_srp_ctx;
#endif
    } ctx;
} oberon_pake_operation_t;


psa_status_t oberon_pake_setup(
    oberon_pake_operation_t *operation,
    const psa_pake_cipher_suite_t *cipher_suite,
    const psa_key_attributes_t *attributes,
    const uint8_t *password, size_t password_length,
    const uint8_t *user_id, size_t user_id_length,
    const uint8_t *peer_id, size_t peer_id_length,
    psa_pake_role_t role);

psa_status_t oberon_pake_output(
    oberon_pake_operation_t *operation,
    psa_pake_step_t step,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_pake_input(
    oberon_pake_operation_t *operation,
    psa_pake_step_t step,
    const uint8_t *input, size_t input_length);

psa_status_t oberon_pake_get_implicit_key(
    oberon_pake_operation_t *operation,
    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t oberon_pake_abort(
    oberon_pake_operation_t *operation);


#ifdef __cplusplus
}
#endif

#endif
