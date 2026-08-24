#ifndef AEAD_TEST_LOGIC_H__
#define AEAD_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct aead_test_data {
	psa_algorithm_t algorithm;
	psa_key_type_t key_type;
	size_t key_bits;
	size_t nonce_size;
};

extern const struct op aead_keysetup_ops[];
extern const struct op aead_operations[];
extern const struct op aead_multipart_operations[];

#define AEAD_KEYSETUP_OP_COUNT 1
#define AEAD_OPERATION_COUNT 2
#define AEAD_MULTIPART_OPERATION_COUNT 2

#define AEAD_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, key_type_value, \
			     key_bits_value, nonce_size_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct aead_test_data){ \
			.algorithm = (algorithm_value), \
			.key_type = (key_type_value), \
			.key_bits = (key_bits_value), \
			.nonce_size = (nonce_size_value), \
		}, \
		.keysetup = {aead_keysetup_ops, AEAD_KEYSETUP_OP_COUNT}, \
		.singlepart = {aead_operations, AEAD_OPERATION_COUNT}, \
		.multipart = {aead_multipart_operations, AEAD_MULTIPART_OPERATION_COUNT}, \
		.check = aead_check, \
		.cleanup = aead_cleanup, \
	}

int aead_check(void);
void aead_cleanup(void);

#endif /* AEAD_TEST_LOGIC_H__ */
