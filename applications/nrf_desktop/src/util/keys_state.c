/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "keys_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(keys_state, CONFIG_DESKTOP_KEYS_STATE_LOG_LEVEL);

#define INVALID_KEY_IDX		UINT8_MAX


static bool keys_state_is_initialized(const struct keys_state *ks)
{
	return (ks->cnt_max > 0);
}

void keys_state_init(struct keys_state *ks, uint8_t key_cnt_max)
{
	LOG_DBG("ks:%p, key_cnt_max:%" PRIu8, (void *)ks, key_cnt_max);

	ARG_UNUSED(keys_state_is_initialized);

	__ASSERT_NO_MSG(!keys_state_is_initialized(ks));
	__ASSERT_NO_MSG((key_cnt_max > 0) && (key_cnt_max <= ARRAY_SIZE(ks->keys)));

	ks->cnt_max = key_cnt_max;
	keys_state_clear(ks);
}

static struct active_key *get_key(struct keys_state *ks, uint8_t idx, uint16_t key_id)
{
	if (idx >= ks->cnt) {
		return NULL;
	}

	struct active_key *key = &ks->keys[idx];

	return ((key->id == key_id) && (key->press_cnt > 0)) ? key : NULL;
}

static struct active_key *alloc_key(struct keys_state *ks, uint8_t idx, uint16_t key_id)
{
	LOG_DBG("ks:%p, key ID:0x%" PRIx16, (void *)ks, key_id);

	__ASSERT_NO_MSG(ks->cnt <= ks->cnt_max);

	if ((idx == INVALID_KEY_IDX) || (ks->cnt == ks->cnt_max)) {
		return NULL;
	}

	__ASSERT_NO_MSG(idx <= ks->cnt);

	/* Shift active keys to make space for the new key. */
	for (uint8_t i = ks->cnt; i > idx; i--) {
		ks->keys[i] = ks->keys[i - 1];
	}

	struct active_key *key = &ks->keys[idx];

	key->id = key_id;
	key->press_cnt = 0;

	ks->cnt++;

	return key;
}

static uint8_t get_key_idx(struct keys_state *ks, uint16_t key_id)
{
	BUILD_ASSERT(ARRAY_SIZE(ks->keys) <= INVALID_KEY_IDX);

	uint8_t idx;

	for (idx = 0; idx < ks->cnt; idx++) {
		/* Find slot for provided key ID. Active keys are sorted ascending by key ID. */
		if (key_id <= ks->keys[idx].id) {
			break;
		}
	}

	__ASSERT_NO_MSG(idx <= ks->cnt_max);
	return (idx == ks->cnt_max) ? INVALID_KEY_IDX : idx;
}

static void free_key(struct keys_state *ks, uint8_t idx)
{
	__ASSERT_NO_MSG(idx < ks->cnt);

	LOG_DBG("ks:%p, key ID:0x%" PRIx16, (void *)ks, ks->keys[idx].id);

	/* Shift active keys to maintain ascending order. */
	for (uint8_t i = idx; i < (ks->cnt - 1); i++) {
		ks->keys[i] = ks->keys[i + 1];
	}
	ks->cnt--;

	/* Free the last active key. */
	ks->keys[ks->cnt].id = 0;
	ks->keys[ks->cnt].press_cnt = 0;
}

int keys_state_key_update(struct keys_state *ks, uint16_t key_id, bool pressed, bool *ks_changed)
{
	LOG_DBG("ks:%p, key ID:0x%" PRIx16 " %s",
		(void *)ks, key_id, pressed ? "press" : "release");

	__ASSERT_NO_MSG(ks_changed);
	__ASSERT_NO_MSG(keys_state_is_initialized(ks));

	int err = 0;
	uint8_t prev_cnt = ks->cnt;
	uint8_t key_idx = get_key_idx(ks, key_id);
	struct active_key *key = get_key(ks, key_idx, key_id);

	if (key) {
		/* Key already active. */
	} else if (pressed) {
		/* Try to allocate new active key for key press. */
		key = alloc_key(ks, key_idx, key_id);
	}

	if (key) {
		__ASSERT_NO_MSG(pressed || (key->press_cnt > 0));
		key->press_cnt += (pressed ? (1) : (-1));

		if (key->press_cnt == 0) {
			/* Key no longer active. */
			free_key(ks, key_idx);
		}
	} else {
		if (pressed) {
			/* Cannot allocate new active key. */
			err = -ENOBUFS;
		} else {
			/* Released key not active. */
			err = -ENOENT;
		}
	}

	*ks_changed = (prev_cnt != ks->cnt);

	return err;
}

void keys_state_clear(struct keys_state *ks)
{
	LOG_DBG("ks:%p", (void *)ks);

	__ASSERT_NO_MSG(keys_state_is_initialized(ks));

	memset(ks->keys, 0, sizeof(ks->keys));
	ks->cnt = 0;
}

size_t keys_state_keys_get(const struct keys_state *ks, uint16_t *res, size_t res_size)
{
	LOG_DBG("ks:%p", (void *)ks);

	__ASSERT_NO_MSG(ks->cnt <= ks->cnt_max);
	__ASSERT_NO_MSG(res_size >= ks->cnt_max);
	ARG_UNUSED(res_size);

	__ASSERT_NO_MSG(keys_state_is_initialized(ks));

	for (uint8_t i = 0; i < ks->cnt; i++) {
		res[i] = ks->keys[i].id;
	}

	return ks->cnt;
}
