/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef XOF_TEST_LOGIC_H__
#define XOF_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct xof_test_data {
	psa_algorithm_t algorithm;
};

extern const struct op xof_multipart_ops[];

#define XOF_OPERATION_COUNT 1

#define XOF_SUITE_ENTRY(algorithm_name, algorithm_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = NULL, \
		.context = &(const struct xof_test_data){ \
			.algorithm = (algorithm_value), \
		}, \
		.multipart = {xof_multipart_ops, XOF_OPERATION_COUNT}, \
	}

#endif /* XOF_TEST_LOGIC_H__ */

