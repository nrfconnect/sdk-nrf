/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MAC_TEST_LOGIC_H__
#define MAC_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct mac_test_data {
	psa_algorithm_t algorithm;
	psa_key_type_t key_type;
	size_t key_bits;
};

extern const struct op mac_keysetup_ops[];
extern const struct op mac_singlepart_ops[];
extern const struct op mac_operations[];

#define MAC_KEYSETUP_OP_COUNT 1
#define MAC_SINGLEPART_OP_COUNT 1
#define MAC_OPERATION_COUNT   2

#define MAC_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, key_type_value, \
			key_bits_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct mac_test_data){ \
			.algorithm = (algorithm_value), \
			.key_type = (key_type_value), \
			.key_bits = (key_bits_value), \
		}, \
		.keysetup = {mac_keysetup_ops, MAC_KEYSETUP_OP_COUNT}, \
		.singlepart = {mac_singlepart_ops, MAC_SINGLEPART_OP_COUNT}, \
		.multipart = {mac_operations, MAC_OPERATION_COUNT}, \
		.cleanup = mac_cleanup, \
	}

void mac_cleanup(void);

#endif /* MAC_TEST_LOGIC_H__ */

