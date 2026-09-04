#ifndef HASH_TEST_LOGIC_H__
#define HASH_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct hash_test_data {
	psa_algorithm_t algorithm;
	size_t digest_length;
};

extern const struct op hash_singlepart_ops[];
extern const struct op hash_multipart_ops[];

#define HASH_SINGLEPART_OP_COUNT 1
#define HASH_MULTIPART_OP_COUNT 1

#define HASH_SUITE_ENTRY(algorithm_description, algorithm_value) \
	{ \
		.alg = (algorithm_description), \
		.keydesc = NULL, \
		.context = &(const struct hash_test_data){ \
			.algorithm = (algorithm_value), \
			.digest_length = PSA_HASH_LENGTH(algorithm_value), \
		}, \
		.singlepart = {hash_singlepart_ops, HASH_SINGLEPART_OP_COUNT}, \
		.multipart = {hash_multipart_ops, HASH_MULTIPART_OP_COUNT}, \
		.check = hash_check, \
	}

int hash_check(void);

#endif /* HASH_TEST_LOGIC_H__ */
