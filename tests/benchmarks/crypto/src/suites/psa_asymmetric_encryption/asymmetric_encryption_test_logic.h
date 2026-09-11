/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ASYMMETRIC_ENCRYPTION_TEST_LOGIC_H__
#define ASYMMETRIC_ENCRYPTION_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct asymmetric_encryption_test_data {
	psa_algorithm_t algorithm;
};

extern const struct op asymmetric_encryption_keysetup_ops[];
extern const struct op asymmetric_encryption_operations[];

#define ASYMMETRIC_ENCRYPTION_KEYSETUP_OP_COUNT 1
#define ASYMMETRIC_ENCRYPTION_OPERATION_COUNT 2

#define ASYMMETRIC_ENCRYPTION_SUITE_ENTRY(algorithm_name, algorithm_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = "rsa2048", \
		.context = &(const struct asymmetric_encryption_test_data){ \
			.algorithm = (algorithm_value), \
		}, \
		.keysetup = {asymmetric_encryption_keysetup_ops, \
			ASYMMETRIC_ENCRYPTION_KEYSETUP_OP_COUNT}, \
		.singlepart = {asymmetric_encryption_operations, \
			ASYMMETRIC_ENCRYPTION_OPERATION_COUNT}, \
		.check = asymmetric_encryption_check, \
		.cleanup = asymmetric_encryption_cleanup, \
	}

int asymmetric_encryption_check(void);
void asymmetric_encryption_cleanup(void);

#endif /* ASYMMETRIC_ENCRYPTION_TEST_LOGIC_H__ */