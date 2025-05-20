/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_plat_digest_cache.h"
#include <suit_platform_internal.h>
#include <zephyr/kernel.h>

#define MAX_MUTEX_LOCK_TIME_MS (100U)

typedef struct {
	struct zcbor_string component_id;
	enum suit_cose_alg alg_id;
	struct zcbor_string digest;
} suit_plat_digest_cache_entry_t;

static suit_plat_digest_cache_entry_t cache[CONFIG_SUIT_DIGEST_CACHE_SIZE];

K_MUTEX_DEFINE(cache_mutex);

/**
 * @brief Get the cache entry index for the given component id
 */
static int find_entry(const struct zcbor_string *component_id,
		      suit_plat_digest_cache_entry_t **p_entry)
{
	size_t i;

	for (i = 0; i < CONFIG_SUIT_DIGEST_CACHE_SIZE; i++) {
		if (suit_compare_zcbor_strings(component_id, &cache[i].component_id)) {
			*p_entry = &cache[i];
			return SUIT_SUCCESS;
		}
	}

	return SUIT_ERR_MISSING_COMPONENT;
}

int suit_plat_digest_cache_add(struct zcbor_string *component_id, enum suit_cose_alg alg_id,
			       struct zcbor_string *digest)
{
	size_t i;

	if (component_id == NULL || digest == NULL) {
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	/* It may seem a waste to lock the mutex for the whole duration of the loop,
	 * however the probability of functions from this file being reentered from a different
	 * thread is low (IPC request from two different cores in a very short time gap)
	 * and could only occur a few times after the system is booted up.
	 * On the other hand there are multiple potential ways of memory corruption by another
	 * thread, including for example changing the entry for a given component just before
	 * the mutex is locked.
	 * Taking this to account, it does not seem beneficial to complicate the code in order
	 * to shorten the time when the mutex is locked.
	 */
	if (k_mutex_lock(&cache_mutex, K_MSEC(MAX_MUTEX_LOCK_TIME_MS)) != 0) {
		return SUIT_ERR_CRASH;
	}

	for (i = 0; i < CONFIG_SUIT_DIGEST_CACHE_SIZE; i++) {
		if (cache[i].component_id.value == NULL ||
		    suit_compare_zcbor_strings(component_id, &cache[i].component_id)) {
			cache[i].component_id.len = component_id->len;
			cache[i].component_id.value = component_id->value;
			cache[i].alg_id = alg_id;
			cache[i].digest.len = digest->len;
			cache[i].digest.value = digest->value;

			k_mutex_unlock(&cache_mutex);
			return SUIT_SUCCESS;
		}
	}

	k_mutex_unlock(&cache_mutex);

	return SUIT_ERR_OVERFLOW;
}

int suit_plat_digest_cache_remove(struct zcbor_string *component_id)
{
	suit_plat_digest_cache_entry_t *entry;

	if (k_mutex_lock(&cache_mutex, K_MSEC(MAX_MUTEX_LOCK_TIME_MS)) != 0) {
		return SUIT_ERR_CRASH;
	}

	if (find_entry(component_id, &entry) == SUIT_SUCCESS) {
		entry->component_id.len = 0;
		entry->component_id.value = NULL;
		entry->alg_id = 0;
		entry->digest.len = 0;
		entry->digest.value = NULL;
	}

	k_mutex_unlock(&cache_mutex);

	return SUIT_SUCCESS;
}

int suit_plat_digest_cache_compare(const struct zcbor_string *component_id,
				   enum suit_cose_alg alg_id, const struct zcbor_string *digest)
{
	int ret;
	suit_plat_digest_cache_entry_t *entry;

	if (k_mutex_lock(&cache_mutex, K_MSEC(MAX_MUTEX_LOCK_TIME_MS)) != 0) {
		return SUIT_ERR_CRASH;
	}

	ret = find_entry(component_id, &entry);

	if (ret == SUIT_SUCCESS) {
		if (entry->alg_id == alg_id && suit_compare_zcbor_strings(digest, &entry->digest)) {
			ret = SUIT_SUCCESS;
		} else {
			ret = SUIT_FAIL_CONDITION;
		}
	}

	k_mutex_unlock(&cache_mutex);

	return ret;
}

int suit_plat_digest_cache_remove_all(void)
{
	size_t i;

	if (k_mutex_lock(&cache_mutex, K_MSEC(MAX_MUTEX_LOCK_TIME_MS)) != 0) {
		return SUIT_ERR_CRASH;
	}

	for (i = 0; i < CONFIG_SUIT_DIGEST_CACHE_SIZE; i++) {
		cache[i].component_id.len = 0;
		cache[i].component_id.value = NULL;
		cache[i].digest.len = 0;
		cache[i].digest.value = NULL;
	}

	k_mutex_unlock(&cache_mutex);

	return SUIT_SUCCESS;
}

int suit_plat_digest_cache_remove_by_handle(suit_component_t handle)
{
	struct zcbor_string *component_id;

	int ret = suit_plat_component_id_get(handle, &component_id);

	if (ret != SUIT_SUCCESS) {
		return ret;
	}

	return suit_plat_digest_cache_remove(component_id);
}
