#ifndef KEY_DERIVATION_TEST_LOGIC_H__
#define KEY_DERIVATION_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

/*
 * What separates one derivation suite from another. Not the salt or info:
 * nothing depends on their values, so all four suites share one of each.
 */
struct key_derivation_test_data {
	psa_algorithm_t algorithm;
	psa_key_type_t key_type;
	size_t key_bits;
	/* Zero for all but PBKDF2, the only one with a cost input. */
	uint32_t cost;
	size_t output_size;
};

extern const struct op key_derivation_keysetup_ops[];
extern const struct op key_derivation_operations[];

#define KEY_DERIVATION_KEYSETUP_OP_COUNT 1
#define KEY_DERIVATION_OPERATION_COUNT 1

#define KEY_DERIVATION_SUITE_ENTRY(algorithm_name, key_description, algorithm_value, \
				   key_type_value, key_bits_value, cost_value, output_size_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct key_derivation_test_data){ \
			.algorithm = (algorithm_value), \
			.key_type = (key_type_value), \
			.key_bits = (key_bits_value), \
			.cost = (cost_value), \
			.output_size = (output_size_value), \
		}, \
		.keysetup = {key_derivation_keysetup_ops, KEY_DERIVATION_KEYSETUP_OP_COUNT}, \
		.singlepart = {key_derivation_operations, KEY_DERIVATION_OPERATION_COUNT}, \
		.check = key_derivation_check, \
		.cleanup = key_derivation_cleanup, \
	}

int key_derivation_check(void);
void key_derivation_cleanup(void);

#endif /* KEY_DERIVATION_TEST_LOGIC_H__ */
