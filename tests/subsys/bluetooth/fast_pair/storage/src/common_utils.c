/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>

#include "fp_common.h"
#include "fp_storage.h"

#define ACCOUNT_KEY_MAX_CNT	CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX
#define ACCOUNT_KEY_LEN		FP_ACCOUNT_KEY_LEN


void cu_generate_account_key(uint8_t seed, struct fp_account_key *account_key)
{
	memset(account_key->key, seed, ACCOUNT_KEY_LEN);
}

bool cu_check_account_key_seed(uint8_t seed, const struct fp_account_key *account_key)
{
	if (account_key->key[0] != seed) {
		return false;
	}

	uint8_t expected[ACCOUNT_KEY_LEN - 1];

	memset(expected, seed, sizeof(expected));
	zassert_mem_equal(&account_key->key[1], expected, sizeof(expected), "Invalid Account Key");

	return true;
}

static bool unloaded_account_key_find_cb(const struct fp_account_key *account_key, void *context)
{
	zassert_true(false, "Callback should not be called before settings are loaded");

	return false;
}

void cu_account_keys_validate_unloaded(void)
{
	int err;
	struct fp_account_key all_keys[ACCOUNT_KEY_MAX_CNT];
	size_t key_count = ACCOUNT_KEY_MAX_CNT;

	err = fp_storage_account_key_count();
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_account_keys_get(all_keys, &key_count);
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_account_key_find(&all_keys[0], unloaded_account_key_find_cb, NULL);
	zassert_equal(err, -ENODATA, "Expected error before settings load");
}

void cu_account_keys_validate_loaded(uint8_t seed_first, uint8_t stored_cnt)
{
	int res;
	int key_cnt;
	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	key_cnt = fp_storage_account_key_count();

	res = fp_storage_account_keys_get(read_keys, &read_cnt);
	zassert_ok(res, "Getting Account Keys failed");
	zassert_equal(key_cnt, read_cnt, "Invalid key count");
	zassert_equal(key_cnt, MIN(stored_cnt, ACCOUNT_KEY_MAX_CNT), "Invalid key count");

	size_t seed_min = (stored_cnt < ACCOUNT_KEY_MAX_CNT) ?
			  (0) : (stored_cnt - ACCOUNT_KEY_MAX_CNT);
	size_t seed_max = stored_cnt - 1;

	seed_min += seed_first;
	seed_max += seed_first;

	for (size_t i = seed_min; i <= seed_max; i++) {
		bool found = false;

		for (size_t j = 0; j < read_cnt; j++) {
			if (found) {
				zassert_false(cu_check_account_key_seed(i, &read_keys[j]),
					      "Key duplicate found");
			}

			if (cu_check_account_key_seed(i, &read_keys[j])) {
				found = true;
			}
		}

		zassert_true(found, "Key dropped by the Fast Pair storage");
	}
}
