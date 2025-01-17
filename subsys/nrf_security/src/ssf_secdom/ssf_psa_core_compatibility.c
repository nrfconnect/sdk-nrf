/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>

/* This define exists in the psa_crypto.c file, I kept the same
 * name here so that it can be searched the same way.
 * In the psa_crypto.c file this define is the concatenation of
 * PSA_CRYPTO_SUBSYSTEM_DRIVER_WRAPPERS_INITIALIZED (=0x1)|
 * PSA_CRYPTO_SUBSYSTEM_KEY_SLOTS_INITIALIZED       (=0x2)|
 * PSA_CRYPTO_SUBSYSTEM_TRANSACTION_INITIALIZED     (=0x4)
 * Just for conformity I kept the same value here.
 */
#define PSA_CRYPTO_SUBSYSTEM_ALL_INITIALISED (0x7)

/* This function is declared in psa_crypto_core.h */
int psa_can_do_hash(psa_algorithm_t hash_alg)
{
	(void)hash_alg;
	/* No initialization is needed when SSF is used, so just return the
	 * expected value here.
	 */
	return PSA_CRYPTO_SUBSYSTEM_ALL_INITIALISED;
}

/* This function is declared in psa_crypto_core.h */
int psa_can_do_cipher(psa_key_type_t key_type, psa_algorithm_t cipher_alg)
{
	(void)key_type;
	(void)cipher_alg;
	/* No initialization is needed when SSF is used, so just return the
	 * expected value here.
	 */
	return PSA_CRYPTO_SUBSYSTEM_ALL_INITIALISED;
}
