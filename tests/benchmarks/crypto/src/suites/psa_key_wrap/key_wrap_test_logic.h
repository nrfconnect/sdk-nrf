/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef KEY_WRAP_TEST_LOGIC_H__
#define KEY_WRAP_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct key_wrap_test_data {
	psa_algorithm_t algorithm;
	size_t key_bits;
};

extern const struct op key_wrap_keysetup_ops[];
extern const struct op key_wrap_operations[];

#define KEY_WRAP_KEYSETUP_OP_COUNT 2
#define KEY_WRAP_OPERATION_COUNT   2

#define KEY_WRAP_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, key_bits_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct key_wrap_test_data){ \
			.algorithm = (algorithm_value), \
			.key_bits = (key_bits_value), \
		}, \
		.keysetup = {key_wrap_keysetup_ops, KEY_WRAP_KEYSETUP_OP_COUNT}, \
		.singlepart = {key_wrap_operations, KEY_WRAP_OPERATION_COUNT}, \
		.check = key_wrap_check, \
		.cleanup = key_wrap_cleanup, \
	}

int key_wrap_check(void);
void key_wrap_cleanup(void);

#endif /* KEY_WRAP_TEST_LOGIC_H__ */

