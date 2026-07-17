/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <dult/dult.h>

#include "dult_user_slot.h"

/** @brief Number of per-user memory references in the slot table.
 *
 *  One memory reference per feature module that keeps per-user state.
 */
#define MEM_REF_COUNT					\
	(IS_ENABLED(CONFIG_DULT_BT_ANOS) +		\
	 IS_ENABLED(CONFIG_DULT_SOUND) +		\
	 IS_ENABLED(CONFIG_DULT_ID) +			\
	 IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR) +	\
	 IS_ENABLED(CONFIG_DULT_BATTERY) +		\
	 IS_ENABLED(CONFIG_DULT_NEAR_OWNER_STATE) +	\
	 IS_ENABLED(CONFIG_DULT_MULTI_USER))

BUILD_ASSERT((MEM_REF_COUNT > 0) && (MEM_REF_COUNT < DULT_USER_SLOT_MEM_REF_ID_UNSET),
	"MEM_REF_COUNT must be greater than 0 and less than DULT_USER_SLOT_MEM_REF_ID_UNSET");

/** Per-user DULT-private state. A slot is bound while .user is non-NULL. */
struct dult_user_slot {
	/** Bound user pointer. NULL when the slot is free. */
	const struct dult_user *user;

	/** Per-user memory references, one per registered id. NULL when unset. */
	void *mem_refs[MEM_REF_COUNT];
};

/* The slot table. */
static struct dult_user_slot slots[CONFIG_DULT_MULTI_USER_MAX];

/* Number of memory references registered so far. */
static size_t mem_ref_count;

static struct dult_user_slot *slot_find(const struct dult_user *user)
{
	if (!user) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].user == user) {
			return &slots[i];
		}
	}

	return NULL;
}

bool dult_user_slot_is_claimed(const struct dult_user *user)
{
	return slot_find(user) != NULL;
}

size_t dult_user_slot_claimed_count(void)
{
	size_t count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].user != NULL) {
			count++;
		}
	}

	return count;
}

int dult_user_slot_claim(const struct dult_user *user)
{
	if (slot_find(user)) {
		/* One slot per user: re-claiming by the same user is a no-op. */
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].user == NULL) {
			slots[i].user = user;
			memset(slots[i].mem_refs, 0, sizeof(slots[i].mem_refs));
			return 0;
		}
	}

	return -ENOMEM;
}

void dult_user_slot_release(const struct dult_user *user)
{
	struct dult_user_slot *slot = slot_find(user);

	if (!slot) {
		return;
	}

	slot->user = NULL;
	memset(slot->mem_refs, 0, sizeof(slot->mem_refs));
}

void dult_user_slot_foreach(void (*cb)(const struct dult_user *user, void *user_data),
			    void *user_data)
{
	/* Re-entrancy guard only: a nested dult_user_slot_foreach() is forbidden. Slot
	 * mutation from within a callback is made safe by the snapshot taken below.
	 */
	static bool in_foreach;

	const struct dult_user *snapshot[CONFIG_DULT_MULTI_USER_MAX];
	size_t count = 0;

	__ASSERT_NO_MSG(cb);
	__ASSERT_NO_MSG(!in_foreach);

	in_foreach = true;

	/* Snapshot the bound users before dispatch: a callback may release its own (or
	 * another) slot, so the delivery loop must not walk the live table.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].user != NULL) {
			snapshot[count++] = slots[i].user;
		}
	}

	/* Re-validate by pointer: a user unregistered by an earlier callback is skipped.
	 * dult_user pointers are caller-owned and stable, so the match is exact.
	 */
	for (size_t i = 0; i < count; i++) {
		if (dult_user_slot_is_claimed(snapshot[i])) {
			cb(snapshot[i], user_data);
		}
	}

	in_foreach = false;
}

int dult_user_slot_mem_ref_register(size_t *id)
{
	__ASSERT_NO_MSG(id);

	if (mem_ref_count >= MEM_REF_COUNT) {
		return -ENOMEM;
	}

	*id = mem_ref_count++;

	return 0;
}

int dult_user_slot_mem_ref_get(const struct dult_user *user, size_t id, void **ref)
{
	struct dult_user_slot *slot = slot_find(user);

	__ASSERT_NO_MSG(id < MEM_REF_COUNT);
	__ASSERT_NO_MSG(ref);

	if (!slot) {
		return -ENODEV;
	}

	*ref = slot->mem_refs[id];

	return 0;
}

int dult_user_slot_mem_ref_set(const struct dult_user *user, size_t id, void *ref)
{
	struct dult_user_slot *slot = slot_find(user);

	__ASSERT_NO_MSG(id < MEM_REF_COUNT);

	if (!slot) {
		return -ENODEV;
	}

	slot->mem_refs[id] = ref;

	return 0;
}

BUILD_ASSERT(sizeof(void *) >= sizeof(uint32_t),
	     "uint32_t must fit in a slot's void * cell");

int dult_user_slot_mem_ref_val_set(const struct dult_user *user, size_t id, uint32_t val)
{
	if (val > DULT_USER_SLOT_MEM_REF_VAL_MAX) {
		return -EINVAL;
	}

	/* Bias by 1 so a cleared cell (NULL) reads back as "unset" rather than as a
	 * stored 0. The range check above prevents the +1 from wrapping.
	 */
	return dult_user_slot_mem_ref_set(user, id, UINT_TO_POINTER(val + 1));
}

int dult_user_slot_mem_ref_val_get(const struct dult_user *user, size_t id, uint32_t *val)
{
	int err;
	void *ref;

	__ASSERT_NO_MSG(val);

	err = dult_user_slot_mem_ref_get(user, id, &ref);
	if (err) {
		return err;
	}

	if (!ref) {
		return -ENOENT;
	}

	*val = (uint32_t)(POINTER_TO_UINT(ref) - 1);

	return 0;
}
