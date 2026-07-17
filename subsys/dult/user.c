/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>

#include <dult/dult.h>
#include <dult/multi_user.h>

#include "dult_battery.h"
#include "dult_bt_anos.h"
#include "dult_id.h"
#include "dult_motion_detector.h"
#include "dult_sound.h"
#include "dult_user.h"
#include "dult_user_slot.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_user, CONFIG_DULT_LOG_LEVEL);

/* The DULT user that called dult_enable(). */
static const struct dult_user *associated_user;

/* Per-user memory reference holding the multi-user coexistence callback.
 * Only used in multi-user builds; the accessors below are reached exclusively
 * from IS_ENABLED(CONFIG_DULT_MULTI_USER) guarded paths.
 */
static size_t multi_user_cb_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;

static const struct dult_multi_user_cb *multi_user_cb_get(const struct dult_user *user)
{
	void *ref = NULL;

	if (multi_user_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return NULL;
	}

	(void) dult_user_slot_mem_ref_get(user, multi_user_cb_id, &ref);

	return ref;
}

static bool small_accessory_capabilities_verify(const struct dult_user *user)
{
	if (!(user->accessory_capabilities & BIT(DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS))) {
		LOG_ERR("DULT play sound capability not supported");
		return false;
	}

	if (!(user->accessory_capabilities &
	      BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_NFC_BIT_POS)) &&
	    !(user->accessory_capabilities &
	      BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS))) {
		LOG_ERR("DULT ID lookup (BLE or NFC) capability not supported");
		return false;
	}

	return true;
}

static int user_struct_validate(const struct dult_user *user)
{
	if (!user) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_ASSERT)) {
		size_t name_len;

		name_len = strnlen(user->manufacturer_name, DULT_USER_STR_PARAM_LEN_MAX + 1);
		__ASSERT_NO_MSG((name_len > 0) && (name_len <= DULT_USER_STR_PARAM_LEN_MAX));

		name_len = strnlen(user->model_name, DULT_USER_STR_PARAM_LEN_MAX + 1);
		__ASSERT_NO_MSG((name_len > 0) && (name_len <= DULT_USER_STR_PARAM_LEN_MAX));
	}

	if (IS_ENABLED(CONFIG_DULT_ACCESSORY_TYPE_SMALL) &&
	    !small_accessory_capabilities_verify(user)) {
		return -EINVAL;
	}

	return 0;
}

int dult_user_register(const struct dult_user *user)
{
	int err;

	err = user_struct_validate(user);
	if (err) {
		return err;
	}

	if (dult_user_is_registered(user)) {
		LOG_ERR("DULT user already registered");
		return -EALREADY;
	}

	err = dult_user_slot_claim(user);
	if (err) {
		/* Single-user: slot taken by another user (legacy -EALREADY). */
		LOG_ERR("DULT user registration failed: %d", err);
		return IS_ENABLED(CONFIG_DULT_MULTI_USER) ? err : -EALREADY;
	}

	return 0;
}

bool dult_user_is_registered(const struct dult_user *user)
{
	return dult_user_slot_is_claimed(user);
}

const struct dult_user *dult_user_get_associated(void)
{
	return associated_user;
}

bool dult_is_any_associated(void)
{
	return associated_user != NULL;
}

bool dult_user_is_associated(const struct dult_user *user)
{
	return user != NULL && associated_user == user;
}

/* Notify every bound user about the arbitration outcome: the winner that it
 * claimed ownership (is_owner == true) and every other bound user that it was
 * evicted (is_owner == false). The evicted slots stay bound; the losing
 * integrations vacate them later via dult_user_unregister().
 */
static void claimed_notify_cb(const struct dult_user *user, void *user_data)
{
	const struct dult_user *winner = user_data;
	const struct dult_multi_user_cb *cb = multi_user_cb_get(user);

	if (cb && cb->ownership_claimed) {
		cb->ownership_claimed(user, user == winner);
	}
}

/* Notify every registered user that the association was released: the
 * releasing user (was_owner == true) and every other registered user that it
 * can now re-arbitrate (was_owner == false).
 */
static void released_notify_cb(const struct dult_user *user, void *user_data)
{
	const struct dult_user *released = user_data;
	const struct dult_multi_user_cb *cb = multi_user_cb_get(user);

	if (cb && cb->ownership_released) {
		cb->ownership_released(user, user == released);
	}
}

/* Arbitration fires while iterating the user slots and typically from a Bluetooth RX /
 * provisioning context. Defer the fan-out to the system workqueue so a consumer may drive
 * the DULT lifecycle from its callback and so slot mutation during the walk is safe.
 * Transitions are delivered in order; if the queue fills before a drain, a cancelling
 * (claim then release of the same user) pair is collapsed rather than delivered.
 */
struct arb_evt {
	const struct dult_user *user;
	bool claimed;
};

/* Depth 2: transitions alternate per slot, so at most one cancelling (same user, opposite)
 * pair can pile up before arb_work drains; a would-be third event collapses that pair.
 */
#define ARB_MSGQ_DEPTH 2

K_MSGQ_DEFINE(arb_msgq, sizeof(struct arb_evt), ARB_MSGQ_DEPTH, __alignof__(struct arb_evt));

static void arb_work_handle(struct k_work *work)
{
	struct arb_evt evt;

	ARG_UNUSED(work);

	while (k_msgq_get(&arb_msgq, &evt, K_NO_WAIT) == 0) {
		dult_user_slot_foreach(evt.claimed ? claimed_notify_cb : released_notify_cb,
				       (void *)evt.user);
	}
}

static K_WORK_DEFINE(arb_work, arb_work_handle);

static void arb_notify_schedule(const struct dult_user *user, bool claimed)
{
	struct arb_evt new_item = {
		.user = user,
		.claimed = claimed,
	};
	struct arb_evt first_item, second_item;

	if (k_msgq_put(&arb_msgq, &new_item, K_NO_WAIT) == 0) {
		(void)k_work_submit(&arb_work);
		return;
	}

	/* Queue full (two events first_item, second_item already pending, new_item incoming).
	 * Pop both and keep only the survivors, collapsing the one cancelling pair that the
	 * alternating claim/release ordering can produce.
	 */
	(void)k_msgq_get(&arb_msgq, &first_item, K_NO_WAIT);
	(void)k_msgq_get(&arb_msgq, &second_item, K_NO_WAIT);

	if (second_item.user == new_item.user && second_item.claimed != new_item.claimed) {
		/* Scenario: queued [reset(A), enable(B)] + incoming reset(B).
		 * Newest pair (second_item=enable(B), new_item=reset(B)) cancels;
		 * deliver only first_item=reset(A).
		 */
		LOG_WRN("DULT: arbitration queue full, dropping cancelling pair (user %p)",
			(void *)new_item.user);
		(void)k_msgq_put(&arb_msgq, &first_item, K_NO_WAIT);
	} else if (first_item.user == second_item.user &&
		   first_item.claimed != second_item.claimed) {
		/* Scenario: queued [enable(A), reset(A)] or [reset(A), enable(A)].
		 * Oldest pair (first_item, second_item) cancels;
		 * deliver only the incoming new_item.
		 */
		LOG_WRN("DULT: arbitration queue full, dropping cancelling pair (user %p)",
			(void *)first_item.user);
		(void)k_msgq_put(&arb_msgq, &new_item, K_NO_WAIT);
	} else {
		/* Unreachable: enable only claims from idle and reset only releases the
		 * associated user, so any three consecutive events contain an adjacent
		 * cancelling pair. Reaching this means the expected event ordering no longer
		 * holds; do not continue on corrupted ownership state.
		 */
		LOG_ERR("DULT: unexpected arbitration event sequence: "
			"first_item(user %p, claimed %d), "
			"second_item(user %p, claimed %d), "
			"new_item(user %p, claimed %d)",
			(void *)first_item.user, first_item.claimed,
			(void *)second_item.user, second_item.claimed,
			(void *)new_item.user, new_item.claimed);
		k_panic();
	}

	(void)k_work_submit(&arb_work);
}

static void ownership_claimed_notify(const struct dult_user *winner)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DULT_MULTI_USER));
	arb_notify_schedule(winner, true);
}

static void ownership_released_notify(const struct dult_user *released)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DULT_MULTI_USER));
	arb_notify_schedule(released, false);
}

int dult_multi_user_cb_register(const struct dult_user *user,
				const struct dult_multi_user_cb *cb)
{
	int err;
	void *ref = NULL;

	if (!IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		return -ENOTSUP;
	}

	if (!cb) {
		return -EINVAL;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (multi_user_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&multi_user_cb_id);
		if (err) {
			return err;
		}
	}

	/* Stored per user, persists across dult_reset(); cleared by
	 * dult_user_unregister(). Registering twice for the same user is rejected.
	 */
	(void) dult_user_slot_mem_ref_get(user, multi_user_cb_id, &ref);
	if (ref) {
		LOG_ERR("DULT multi-user: callbacks already registered");
		return -EALREADY;
	}

	return dult_user_slot_mem_ref_set(user, multi_user_cb_id, (void *)cb);
}

/* Release associated-user state (the singletons enabled by dult_enable()). Runs the
 * per-feature reset sequence in the same order as the pre-multi-user
 * dult_reset(), returning the first error encountered so the single-user
 * terminal reset can propagate it unchanged.
 */
static int associated_release(const struct dult_user *user)
{
	int err;

	if (user->accessory_capabilities & BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS)) {
		err = dult_id_reset();
		if (err) {
			LOG_ERR("dult_id_reset returned an error: %d", err);
			return err;
		}
	}

	if (user->accessory_capabilities & BIT(DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS)) {
		err = dult_sound_reset();
		if (err) {
			LOG_ERR("dult_sound_reset returned an error: %d", err);
			return err;
		}
	}

	err = dult_bt_anos_reset();
	if (err) {
		LOG_ERR("dult_bt_anos_reset returned an error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR) &&
	    (user->accessory_capabilities &
	     BIT(DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS))) {
		err = dult_motion_detector_reset();
		if (err) {
			LOG_ERR("dult_motion_detector_reset returned an error: %d", err);
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_DULT_BATTERY)) {
		err = dult_battery_reset();
		if (err) {
			LOG_ERR("dult_battery_reset returned an error: %d", err);
			return err;
		}
	}

	return 0;
}

int dult_user_unregister(const struct dult_user *user)
{
	if (!IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!dult_user_is_registered(user)) {
		return 0;
	}

	if (associated_user == user) {
		return -EACCES;
	}

	dult_user_slot_release(user);

	/* Drop any undelivered arbitration events once the final user leaves. */
	if (dult_user_slot_claimed_count() == 0) {
		k_msgq_purge(&arb_msgq);
		(void)k_work_cancel(&arb_work);
	}

	LOG_INF("DULT user unregistered");

	return 0;
}

int dult_enable(const struct dult_user *user)
{
	int err;

	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (associated_user == user) {
		LOG_ERR("DULT already enabled");
		return -EALREADY;
	}

	if (associated_user != NULL) {
		LOG_ERR("DULT already enabled by another user");
		return -EACCES;
	}

	/* Publish the association up front so the per-feature enable hooks can
	 * resolve the winning user's callbacks (stored per user in the slot's
	 * memory references) through dult_user_get_associated(). Rolled back on any
	 * failure below.
	 */
	associated_user = user;

	if (IS_ENABLED(CONFIG_DULT_BATTERY)) {
		err = dult_battery_enable();
		if (err) {
			LOG_ERR("dult_battery_enable returned an error: %d", err);
			associated_user = NULL;
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR) &&
	    (user->accessory_capabilities &
	     BIT(DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS))) {
		err = dult_motion_detector_enable();
		if (err) {
			LOG_ERR("dult_motion_detector_enable returned an error: %d", err);
			associated_user = NULL;
			return err;
		}
	}

	err = dult_bt_anos_enable();
	if (err) {
		LOG_ERR("dult_bt_anos_enable returned an error: %d", err);
		return err;
	}

	if (user->accessory_capabilities & BIT(DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS)) {
		err = dult_sound_enable();
		if (err) {
			LOG_ERR("dult_sound_enable returned an error: %d", err);
			associated_user = NULL;
			return err;
		}
	}

	if (user->accessory_capabilities & BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS)) {
		err = dult_id_enable();
		if (err) {
			LOG_ERR("dult_id_enable returned an error: %d", err);
			associated_user = NULL;
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		/* Announce the arbitration outcome: winner claimed, others evicted. */
		ownership_claimed_notify(user);
	}

	LOG_INF("DULT enabled");

	return 0;
}

int dult_reset(const struct dult_user *user)
{
	int err;
	bool user_authorized;

	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	user_authorized = IS_ENABLED(CONFIG_DULT_MULTI_USER) ?
			  (associated_user == user) : dult_user_is_registered(user);
	if (!user_authorized) {
		return -EACCES;
	}

	err = associated_release(user);
	if (err) {
		return err;
	}

	associated_user = NULL;

	if (IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		ownership_released_notify(user);
		LOG_INF("DULT reset (slot retained)");
	} else {
		/* Terminal teardown. */
		dult_user_slot_release(user);
		LOG_INF("DULT reset completed");
	}

	return 0;
}
