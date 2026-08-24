#ifndef KEY_AGREEMENT_TEST_LOGIC_H__
#define KEY_AGREEMENT_TEST_LOGIC_H__

#include "../../crypto_benchmarks.h"

struct key_agreement_test_data {
	psa_ecc_family_t family;
	size_t bits;
	size_t public_key_size;
	size_t secret_size;
};

extern const struct op key_agreement_keysetup_ops[];
extern const struct op key_agreement_operations[];

/* Written out because the arrays live in another translation unit, where
 * ARRAY_SIZE cannot reach them.
 */
#define KEY_AGREEMENT_KEYSETUP_OP_COUNT 2
#define KEY_AGREEMENT_OPERATION_COUNT 1

#define KEY_AGREEMENT_SUITE_ENTRY(algorithm_name, key_description, family_value, bits_value, \
				       public_key_size_value, secret_size_value) \
	{ \
		.alg = (algorithm_name), \
		.keydesc = (key_description), \
		.context = &(const struct key_agreement_test_data){ \
			.family = (family_value), \
			.bits = (bits_value), \
			.public_key_size = (public_key_size_value), \
			.secret_size = (secret_size_value), \
		}, \
		.keysetup = {key_agreement_keysetup_ops, KEY_AGREEMENT_KEYSETUP_OP_COUNT}, \
		.singlepart = {key_agreement_operations, KEY_AGREEMENT_OPERATION_COUNT}, \
		.check = key_agreement_check, \
		.cleanup = key_agreement_cleanup, \
	}

int key_agreement_check(void);
void key_agreement_cleanup(void);

#endif /* KEY_AGREEMENT_TEST_LOGIC_H__ */
