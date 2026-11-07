/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SIGNATURE_TEST_LOGIC_H__
#define SIGNATURE_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct signature_test_data {
	psa_algorithm_t algorithm;
	psa_key_type_t key_type;
	size_t key_bits;
	/* Zero for algorithms that sign the message itself, such as pure EdDSA. */
	psa_algorithm_t hash_algorithm;
};

extern const struct op signature_keysetup_ops[];
extern const struct op signature_operations[];

#define SIGNATURE_KEYSETUP_OP_COUNT 1
#define SIGNATURE_OPERATION_COUNT   2

#define SIGNATURE_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, key_type_value, \
			      key_bits_value, hash_algorithm_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct signature_test_data){ \
			.algorithm = (algorithm_value), \
			.key_type = (key_type_value), \
			.key_bits = (key_bits_value), \
			.hash_algorithm = (hash_algorithm_value), \
		}, \
		.keysetup = {signature_keysetup_ops, SIGNATURE_KEYSETUP_OP_COUNT}, \
		.singlepart = {signature_operations, SIGNATURE_OPERATION_COUNT}, \
		.cleanup = signature_cleanup, \
	}

#define ECDSA_SUITE_ENTRY(key_description, curve_family, curve_bits, hash_algorithm_value) \
	SIGNATURE_SUITE_ENTRY("ecdsa", key_description, PSA_ALG_ECDSA(hash_algorithm_value), \
			      PSA_KEY_TYPE_ECC_KEY_PAIR(curve_family), curve_bits, \
			      hash_algorithm_value)

#define RSA_PSS_SUITE_ENTRY(key_description, hash_algorithm_value) \
	SIGNATURE_SUITE_ENTRY("rsa_pss", key_description, PSA_ALG_RSA_PSS(hash_algorithm_value), \
			      PSA_KEY_TYPE_RSA_KEY_PAIR, 2048, hash_algorithm_value)

extern const struct op ml_dsa_keysetup_ops[];
extern const struct op ml_dsa_operations[];

#define ML_DSA_KEYSETUP_OP_COUNT 1
#define ML_DSA_OPERATION_COUNT	 1

/* Verify-only, over a fixed vector: see ml_dsa_import_key(). */
#define ML_DSA_VERIFY_SUITE_ENTRY(key_description) \
	{ \
		.alg = "ml_dsa", \
		.keydesc = (key_description), \
		.keysetup = {ml_dsa_keysetup_ops, ML_DSA_KEYSETUP_OP_COUNT}, \
		.singlepart = {ml_dsa_operations, ML_DSA_OPERATION_COUNT}, \
		.cleanup = signature_cleanup, \
	}

void signature_cleanup(void);

#endif /* SIGNATURE_TEST_LOGIC_H__ */
