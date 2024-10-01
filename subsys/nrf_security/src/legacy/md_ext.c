/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <psa/crypto.h>

/* Functions required for MD to work. These APIs are copied from psa_crypto.c*/

int psa_can_do_hash(psa_algorithm_t hash_alg)
{
    (void) hash_alg;
    return 1;
}

int psa_can_do_cipher(psa_key_type_t key_type, psa_algorithm_t cipher_alg)
{
    (void) key_type;
    (void) cipher_alg;
    return 1;
}
