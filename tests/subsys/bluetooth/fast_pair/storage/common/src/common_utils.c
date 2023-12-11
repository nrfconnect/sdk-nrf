/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include "fp_common.h"
#include "fp_storage_ak.h"

#define ACCOUNT_KEY_MAX_CNT	CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX
#define ACCOUNT_KEY_LEN		FP_ACCOUNT_KEY_LEN


void cu_generate_account_key(uint8_t seed, struct fp_account_key *account_key)
{
	memset(account_key->key, seed, ACCOUNT_KEY_LEN);
}

void cu_account_keys_generate_and_store(uint8_t first_seed, uint8_t gen_count)
{
	int err;
	struct fp_account_key account_key;

	zassert_true((first_seed + gen_count) <= (UCHAR_MAX + 1), "Invalid seed range");

	for (uint8_t i = 0; i < gen_count; i++) {
		cu_generate_account_key(i + first_seed, &account_key);

		err = fp_storage_ak_save(&account_key);
		zassert_ok(err, "Failed to store Account Key");
	}
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

static bool invalid_account_key_find_cb(const struct fp_account_key *account_key, void *context)
{
	zassert_true(false, "Callback should never be called when settings are not initialized");

	return false;
}

void cu_account_keys_validate_uninitialized(void)
{
	static const uint8_t seed;

	int err;
	struct fp_account_key account_key;
	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	cu_generate_account_key(seed, &account_key);
	err = fp_storage_ak_save(&account_key);
	zassert_equal(err, -EACCES, "Expected error before initialization");

	err = fp_storage_ak_count();
	zassert_equal(err, -EACCES, "Expected error before initialization");

	err = fp_storage_ak_get(read_keys, &read_cnt);
	zassert_equal(err, -EACCES, "Expected error before initialization");

	err = fp_storage_ak_find(NULL, invalid_account_key_find_cb, NULL);
	zassert_equal(err, -EACCES, "Expected error before initialization");
}

void cu_account_keys_validate_loaded(uint8_t seed_first, uint8_t stored_cnt)
{
	int res;
	int key_cnt;
	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	key_cnt = fp_storage_ak_count();

	res = fp_storage_ak_get(read_keys, &read_cnt);
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
