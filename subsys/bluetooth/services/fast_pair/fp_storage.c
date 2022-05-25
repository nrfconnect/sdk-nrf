/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/settings/settings.h>

#include "fp_storage.h"
#include "fp_crypto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#define SUBTREE_NAME "fp"
#define SETTINGS_AK_NAME_PREFIX "ak_data"
#define SETTINGS_NAME_CONNECTOR "/"
#define SETTINGS_AK_FULL_PREFIX (SUBTREE_NAME SETTINGS_NAME_CONNECTOR SETTINGS_AK_NAME_PREFIX)
#define SETTINGS_AK_NAME_MAX_SUFFIX_LEN 1
#define SETTINGS_NAME_MAX_SIZE (sizeof(SETTINGS_AK_FULL_PREFIX) + SETTINGS_AK_NAME_MAX_SUFFIX_LEN)

#define ACCOUNT_KEY_CNT    CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX
#define ACCOUNT_KEY_MIN_ID 1
#define ACCOUNT_KEY_MAX_ID (2 * ACCOUNT_KEY_CNT)

BUILD_ASSERT(ACCOUNT_KEY_MAX_ID < UINT8_MAX);
BUILD_ASSERT(ACCOUNT_KEY_CNT <= 10);

struct account_key_data {
	uint8_t account_key_id;
	uint8_t account_key[FP_CRYPTO_ACCOUNT_KEY_LEN];
};

static uint8_t account_key_list[ACCOUNT_KEY_CNT][FP_CRYPTO_ACCOUNT_KEY_LEN];
static uint8_t account_key_loaded_ids[ACCOUNT_KEY_CNT];
static uint8_t account_key_next_id;
static uint8_t account_key_count;
static atomic_t settings_loaded = ATOMIC_INIT(false);


static uint8_t key_id_to_idx(uint8_t account_key_id)
{
	__ASSERT_NO_MSG(account_key_id >= ACCOUNT_KEY_MIN_ID);

	return (account_key_id - ACCOUNT_KEY_MIN_ID) % ACCOUNT_KEY_CNT;
}

static uint8_t next_key_id(uint8_t key_id)
{
	if (key_id == ACCOUNT_KEY_MAX_ID) {
		return ACCOUNT_KEY_MIN_ID;
	}

	return key_id + 1;
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	uint8_t index;
	struct account_key_data data;
	const char *key = SETTINGS_AK_NAME_PREFIX;
	const char *name_suffix;
	size_t name_suffix_len;

	if (!strncmp(name, key, sizeof(SETTINGS_AK_NAME_PREFIX) - 1)) {
		if (len != sizeof(data)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &data, sizeof(data));
		if (rc < 0) {
			return rc;
		}

		if (rc != sizeof(data)) {
			return -EINVAL;
		}

		if ((data.account_key_id < ACCOUNT_KEY_MIN_ID) ||
		    (data.account_key_id > ACCOUNT_KEY_MAX_ID)) {
			return -EINVAL;
		}

		index = key_id_to_idx(data.account_key_id);
		name_suffix = &name[sizeof(SETTINGS_AK_NAME_PREFIX) - 1];
		name_suffix_len = strlen(name_suffix);

		if ((name_suffix_len < 1) || (name_suffix_len > SETTINGS_AK_NAME_MAX_SUFFIX_LEN)) {
			return -EINVAL;
		}

		for (size_t i = 0; i < strlen(name_suffix); i++) {
			if (!isdigit(name_suffix[i])) {
				return -EINVAL;
			}
		}

		if (index != atoi(name_suffix)) {
			return -EINVAL;
		}

		if (account_key_loaded_ids[index] != 0) {
			return -EINVAL;
		}

		memcpy(&account_key_list[index], data.account_key, sizeof(data.account_key));
		account_key_loaded_ids[index] = data.account_key_id;

		return 0;
	}

	return -ENOENT;
}

static bool zero_id_check(uint8_t start_idx)
{
	for (size_t i = start_idx; i < ACCOUNT_KEY_CNT; i++) {
		if (account_key_loaded_ids[i] != 0) {
			return false;
		}
	}

	return true;
}

static bool id_sequence_check(uint8_t start_idx)
{
	__ASSERT_NO_MSG(start_idx > 0);
	for (size_t i = start_idx; i < ACCOUNT_KEY_CNT; i++) {
		if (account_key_loaded_ids[i] != next_key_id(account_key_loaded_ids[i - 1])) {
			return false;
		}
	}

	return true;
}

static bool rollover_check(uint8_t cur_id, uint8_t prev_id)
{
	return ((cur_id == (prev_id - (ACCOUNT_KEY_CNT - 1))) ||
		(cur_id == (prev_id + (ACCOUNT_KEY_CNT + 1))));
}

static int fp_settings_commit(void)
{
	uint8_t prev_id;
	uint8_t cur_id;
	int first_zero_idx = -1;
	int rollover_idx = -1;

	cur_id = account_key_loaded_ids[0];
	if ((cur_id != 0) && (key_id_to_idx(cur_id) != 0)) {
		return -EINVAL;
	}

	if (cur_id == 0) {
		first_zero_idx = 0;
	}

	for (size_t i = 1; (i < ACCOUNT_KEY_CNT) &&
			   (first_zero_idx == -1); i++) {
		prev_id = cur_id;
		cur_id = account_key_loaded_ids[i];
		if (cur_id == 0) {
			if (prev_id != (i - 1 + ACCOUNT_KEY_MIN_ID)) {
				return -EINVAL;
			}

			first_zero_idx = i;
			break;
		}
		if (cur_id != next_key_id(prev_id)) {
			if (!rollover_check(cur_id, prev_id)) {
				return -EINVAL;
			}

			rollover_idx = i;
			break;
		}
	}

	if (first_zero_idx != -1) {
		if (!zero_id_check(first_zero_idx + 1)) {
			return -EINVAL;
		}

		account_key_count = first_zero_idx;
		account_key_next_id = first_zero_idx + ACCOUNT_KEY_MIN_ID;
	} else if (rollover_idx != -1) {
		if (!id_sequence_check(rollover_idx + 1)) {
			return -EINVAL;
		}

		account_key_count = ACCOUNT_KEY_CNT;
		account_key_next_id = next_key_id(account_key_loaded_ids[rollover_idx - 1]);
	} else {
		account_key_count = ACCOUNT_KEY_CNT;
		account_key_next_id = next_key_id(account_key_loaded_ids[ACCOUNT_KEY_CNT - 1]);
	}

	atomic_set(&settings_loaded, true);

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(fast_pair_storage, SUBTREE_NAME, NULL, fp_settings_set,
			       fp_settings_commit, NULL);

int fp_storage_account_key_count(void)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	return account_key_count;
}

int fp_storage_account_keys_get(uint8_t buf[][FP_CRYPTO_ACCOUNT_KEY_LEN], size_t *key_count)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	if (*key_count < account_key_count) {
		return -EINVAL;
	}

	memcpy(buf, account_key_list, account_key_count * FP_CRYPTO_ACCOUNT_KEY_LEN);
	*key_count = account_key_count;

	return 0;
}

int fp_storage_account_key_save(const uint8_t *account_key)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	uint8_t index;
	struct account_key_data data;
	char name[SETTINGS_NAME_MAX_SIZE];
	int n;
	int err;

	for (size_t i = 0; i < account_key_count; i++) {
		if (!memcmp(account_key, &account_key_list[i], FP_CRYPTO_ACCOUNT_KEY_LEN)) {
			LOG_INF("Account Key already saved - skipping.");
			return 0;
		}
	}
	if (account_key_count == ACCOUNT_KEY_CNT) {
		LOG_INF("Account Key List full - erasing the oldest Account Key.");
	}

	index = key_id_to_idx(account_key_next_id);

	data.account_key_id = account_key_next_id;
	memcpy(data.account_key, account_key, FP_CRYPTO_ACCOUNT_KEY_LEN);

	n = snprintf(name, SETTINGS_NAME_MAX_SIZE, "%s%u", SETTINGS_AK_FULL_PREFIX, index);
	__ASSERT_NO_MSG(n < SETTINGS_NAME_MAX_SIZE);
	if (n < 0) {
		return n;
	}

	err = settings_save_one(name, &data, sizeof(data));
	if (err) {
		return err;
	}

	memcpy(&account_key_list[index], account_key, FP_CRYPTO_ACCOUNT_KEY_LEN);

	account_key_next_id = next_key_id(account_key_next_id);

	if (account_key_count < ACCOUNT_KEY_CNT) {
		account_key_count++;
	}

	return 0;
}
