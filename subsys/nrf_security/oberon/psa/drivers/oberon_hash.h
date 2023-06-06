/*
 * Copyright (c) 2016 - 2023 Nordic Semiconductor ASA
 * Copyright (c) since 2020 Oberon microsystems AG
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef OBERON_HASH_H
#define OBERON_HASH_H

#include <psa/crypto_driver_common.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint64_t ctx[52];
    psa_algorithm_t alg;
} oberon_hash_operation_t;


psa_status_t oberon_hash_setup(
    oberon_hash_operation_t *operation,
    psa_algorithm_t alg);

psa_status_t oberon_hash_clone(
    const oberon_hash_operation_t *source_operation,
    oberon_hash_operation_t *target_operation);

psa_status_t oberon_hash_update(
    oberon_hash_operation_t *operation,
    const uint8_t *input, size_t input_length);

psa_status_t oberon_hash_finish(
    oberon_hash_operation_t *operation,
    uint8_t *hash, size_t hash_size, size_t *hash_length);

psa_status_t oberon_hash_abort(
    oberon_hash_operation_t *operation);


psa_status_t oberon_hash_compute(
    psa_algorithm_t alg,
    const uint8_t *input, size_t input_length,
    uint8_t *hash, size_t hash_size, size_t *hash_length);


#ifdef __cplusplus
}
#endif

#endif
