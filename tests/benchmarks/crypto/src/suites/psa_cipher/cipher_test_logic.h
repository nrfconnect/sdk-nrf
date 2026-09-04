#ifndef CIPHER_TEST_LOGIC_H__
#define CIPHER_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct cipher_test_data {
	const char *algorithm_name;
	const char *key_description;
	psa_algorithm_t algorithm;
	psa_key_id_t persistent_id;
	size_t key_bits;
	bool has_singlepart;
	bool has_multipart;
};

extern const struct op cipher_keysetup_ops[];
extern const struct op cipher_singlepart_ops[];
extern const struct op cipher_multipart_ops[];

#define CIPHER_KEYSETUP_OP_COUNT 1
#define CIPHER_SINGLEPART_OP_COUNT 2
#define CIPHER_MULTIPART_OP_COUNT 2

#define CIPHER_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, key_bits_value, \
			   has_multipart_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct cipher_test_data){ \
			.algorithm = (algorithm_value), \
			.key_bits = (key_bits_value), \
			.has_singlepart = true, \
			.has_multipart = (has_multipart_value), \
		}, \
		.keysetup = {cipher_keysetup_ops, CIPHER_KEYSETUP_OP_COUNT}, \
		.singlepart = {cipher_singlepart_ops, CIPHER_SINGLEPART_OP_COUNT}, \
		.multipart = {(has_multipart_value) ? cipher_multipart_ops : NULL, \
			(has_multipart_value) ? CIPHER_MULTIPART_OP_COUNT : 0}, \
		.check = cipher_check, \
		.cleanup = cipher_cleanup, \
	}

int cipher_check(void);
void cipher_cleanup(void);

#endif /* CIPHER_TEST_LOGIC_H__ */
