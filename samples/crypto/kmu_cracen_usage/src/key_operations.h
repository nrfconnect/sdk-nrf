/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#ifndef __KEY_OPERATIONS_H_
#define __KEY_OPERATIONS_H_

#define KEY_OPERATIONS_EDDSA_PUBLIC_KEY_SIZE    (32)
#define KEY_OPERATIONS_EDDSA_SIGNATURE_SIZE     (64)

#define KEY_OPERATIONS_ECDSA_PUBLIC_KEY_SIZE    (65)
#define KEY_OPERATIONS_ECDSA_SIGNATURE_SIZE     (64)
#define KEY_OPERATIONS_ECDSA_HASH_SIZE          (32)

/* AES key operations */

int key_operations_generate_aes_key(psa_key_id_t *key_id);
int key_operations_use_aes_key(psa_key_id_t *key_id);

/* ECC key pair operations */

int key_operations_use_eddsa_key_pair(psa_key_id_t *key_pair_id);

int key_operations_generate_ecdsa_key_pair(psa_key_id_t *key_pair_id);
int key_operations_use_ecdsa_key_pair(psa_key_id_t *key_pair_id);

#endif
