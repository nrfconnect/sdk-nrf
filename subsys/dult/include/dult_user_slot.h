/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_USER_SLOT_H_
#define _DULT_USER_SLOT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dult/dult.h>

/**
 * @defgroup dult_user_slot Detecting Unwanted Location Trackers user slots
 * @brief Private API for the DULT user slot table.
 *
 * The slot table is the storage backend shared by the v1 and v2 lifecycle
 * flows implemented in user.c. When only one user slot is configured
 * (@kconfig{CONFIG_DULT_USER_MAX} equal to 1) the table degenerates to a
 * single slot, which makes the single-user state map onto the same storage
 * without any behavioral change.
 *
 * Feature modules keep their per-user memory local to the module and reference it
 * through the memory references of this table. Each module registers one
 * memory-reference id with @ref dult_user_slot_mem_ref_register and then uses
 * @ref dult_user_slot_mem_ref_get / @ref dult_user_slot_mem_ref_set to access
 * its per-user pointer.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Unset memory reference identifier. */
#define DULT_USER_SLOT_MEM_REF_ID_UNSET (0xFF)

/** @brief Claim a slot for @p user.
 *
 *  If @p user already holds a slot, this is a no-op success.
 *
 *  @param user User to bind.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_slot_claim(const struct dult_user *user);

/** @brief Release @p user's slot, returning it to the free state.
 *
 *  Clears every per-user memory reference registered for @p user, so each
 *  feature module's per-user state is wiped in one place. No-op when @p user
 *  does not currently hold a slot.
 *
 *  @param user User whose slot should be released.
 */
void dult_user_slot_release(const struct dult_user *user);

/** @brief Check whether @p user currently holds a slot.
 *
 *  @param user User to look up. NULL is accepted and never matches.
 *
 *  @return true if @p user holds a slot, false otherwise.
 */
bool dult_user_slot_is_claimed(const struct dult_user *user);

/** @brief Get the number of slots currently claimed.
 *
 *  @return Number of bound slots.
 */
size_t dult_user_slot_claimed_count(void);

/** @brief Call @p cb for every user that currently holds a slot.
 *
 *  The bound users are snapshotted at entry, so @p cb may release its own or another
 *  user's slot (a slot released before its turn is skipped). @p cb must not call
 *  @ref dult_user_slot_foreach recursively.
 *
 *  @param cb        Called once per slot bound at entry, in table order, and only if the
 *                    slot is still claimed when its turn comes.
 *  @param user_data Opaque data forwarded to @p cb unchanged.
 */
void dult_user_slot_foreach(void (*cb)(const struct dult_user *user, void *user_data),
			    void *user_data);

/** @brief Register a per-user memory reference and get its id.
 *
 *  Called once per feature module before the first use of the memory reference.
 *  The returned identifier is used with @ref dult_user_slot_mem_ref_get and
 *  @ref dult_user_slot_mem_ref_set.
 *
 *  @param[out] id Identifier of the registered memory reference.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_slot_mem_ref_register(size_t *id);

/** @brief Get the memory reference stored under @p id for @p user.
 *
 *  @param user User to look up.
 *  @param id   Identifier from @ref dult_user_slot_mem_ref_register.
 *  @param ref  Stored memory reference (NULL when unset), written only on success.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_slot_mem_ref_get(const struct dult_user *user, size_t id, void **ref);

/** @brief Set the memory reference stored under @p id for @p user.
 *
 *  @param user User to look up.
 *  @param id   Identifier from @ref dult_user_slot_mem_ref_register.
 *  @param ref  Memory reference to store (NULL clears it).
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_slot_mem_ref_set(const struct dult_user *user, size_t id, void *ref);

/** @brief Maximum value storable via the user-slot value accessors.
 *
 *  The value cell reuses the slot's @c void* storage biased by +1 so a cleared
 *  cell (NULL) reads back as "unset"; this reserves one value of the @c uint32_t
 *  range.
 */
#define DULT_USER_SLOT_MEM_REF_VAL_MAX (UINT32_MAX - 1)

/** @brief Store a small unsigned scalar under @p id for @p user.
 *
 *  Reuses the memory-reference storage rather than a separate table: the value is
 *  packed into the reference pointer and biased by +1, so a cleared cell (NULL,
 *  produced by @ref dult_user_slot_release) reads back as "unset". Consequences:
 *   - an @p id used with the value accessors must never also store a pointer via
 *     @ref dult_user_slot_mem_ref_set, and vice versa;
 *   - @p val must be at most @ref DULT_USER_SLOT_MEM_REF_VAL_MAX;
 *   - the value is wiped together with the slot on release.
 *
 *  @param user User to look up.
 *  @param id   Identifier from @ref dult_user_slot_mem_ref_register.
 *  @param val  Value to store, at most @ref DULT_USER_SLOT_MEM_REF_VAL_MAX.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_slot_mem_ref_val_set(const struct dult_user *user, size_t id, uint32_t val);

/** @brief Get the scalar stored under @p id for @p user.
 *
 *  See @ref dult_user_slot_mem_ref_val_set for the packing/bias convention.
 *
 *  @param user     User to look up.
 *  @param id       Identifier from @ref dult_user_slot_mem_ref_register.
 *  @param[out] val Stored value, written only on success.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_user_slot_mem_ref_val_get(const struct dult_user *user, size_t id, uint32_t *val);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_USER_SLOT_H_ */
