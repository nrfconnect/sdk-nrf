/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <errno.h>

#include "dult_bt_anos.h"
#include "dult_motion_detector.h"
#include "dult_user.h"
#include "dult_user_slot.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_sound, CONFIG_DULT_LOG_LEVEL);

static bool is_enabled;
static bool sound_active;
static enum dult_sound_src sound_src;

/* Per-user memory reference holding the user's sound callback. */
static size_t sound_cb_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;

/* Resolve the sound callback of the currently associated user. */
static const struct dult_sound_cb *sound_cb_get(void)
{
	void *ref = NULL;

	if (sound_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return NULL;
	}

	(void) dult_user_slot_mem_ref_get(dult_user_get_associated(), sound_cb_id, &ref);

	return ref;
}

int dult_sound_cb_register(const struct dult_user *user, const struct dult_sound_cb *cb)
{
	int err;
	void *ref = NULL;

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!cb || !cb->sound_start || !cb->sound_stop) {
		return -EINVAL;
	}

	if (sound_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&sound_cb_id);
		if (err) {
			return err;
		}
	}

	/* The callback is stored per user and persists across dult_reset(); it is
	 * cleared by dult_user_unregister(). Registering twice for the same user
	 * is rejected.
	 */

	(void) dult_user_slot_mem_ref_get(user, sound_cb_id, &ref);
	if (ref) {
		LOG_ERR("DULT Sound: sound callbacks already registered");
		return -EALREADY;
	}

	return dult_user_slot_mem_ref_set(user, sound_cb_id, (void *)cb);
}

int dult_sound_state_update(const struct dult_user *user,
			    const struct dult_sound_state_param *param)
{
	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!dult_is_any_associated()) {
		LOG_ERR("DULT Sound: module is not enabled");
		return -EACCES;
	}
	__ASSERT_NO_MSG(is_enabled);

	if (param->active == sound_active) {
		if (param->src == sound_src) {
			/* The sound module state is unchanged.*/
			LOG_WRN("DULT Sound: state has not changed");
			return 0;
		}

		if (!sound_active) {
			/* The sound source change is irrelevant when sound is not active. */
			LOG_WRN("DULT Sound: unnecessary source change when sound is not active");
			return 0;
		}
	}

	sound_active = param->active;
	sound_src = param->src;

	dult_bt_anos_sound_state_change_notify(param->active,
					       param->src == DULT_SOUND_SRC_BT_GATT);

	if (IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR)) {
		dult_motion_detector_sound_state_change_notify(
						param->active,
						param->src == DULT_SOUND_SRC_MOTION_DETECTOR);
	}

	return 0;
}

static void anos_sound_start(void)
{
	const struct dult_sound_cb *sound_cb = sound_cb_get();

	if (sound_active) {
		dult_bt_anos_sound_state_change_notify(sound_active, false);
		return;
	}

	__ASSERT(sound_cb && sound_cb->sound_start,
		 "DULT Sound: start callback is not populated");

	sound_cb->sound_start(DULT_SOUND_SRC_BT_GATT);
}

static void anos_sound_stop(void)
{
	const struct dult_sound_cb *sound_cb = sound_cb_get();

	__ASSERT(sound_cb && sound_cb->sound_stop,
		 "DULT Sound: stop callback is not populated");

	sound_cb->sound_stop(DULT_SOUND_SRC_BT_GATT);
}

const static struct dult_bt_anos_sound_cb anos_sound_cb = {
	.sound_start = anos_sound_start,
	.sound_stop = anos_sound_stop,
};

static void motion_detector_sound_start(void)
{
	const struct dult_sound_cb *sound_cb = sound_cb_get();

	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR));

	if (sound_active) {
		dult_motion_detector_sound_state_change_notify(sound_active, false);
		return;
	}

	__ASSERT(sound_cb && sound_cb->sound_start,
		 "DULT Sound: start callback is not populated");

	sound_cb->sound_start(DULT_SOUND_SRC_MOTION_DETECTOR);
}

const static struct dult_motion_detector_sound_cb motion_detector_sound_cb = {
	.sound_start = motion_detector_sound_start,
};

int dult_sound_enable(void)
{
	static bool bt_anos_sound_cb_registered;

	if (!bt_anos_sound_cb_registered) {
		dult_bt_anos_sound_cb_register(&anos_sound_cb);
		bt_anos_sound_cb_registered = true;
	}

	if (IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR)) {
		static bool motion_detector_sound_cb_registered;

		if (!motion_detector_sound_cb_registered) {
			dult_motion_detector_sound_cb_register(&motion_detector_sound_cb);
			motion_detector_sound_cb_registered = true;
		}
	}

	if (is_enabled) {
		LOG_ERR("DULT Sound: already enabled");
		return -EALREADY;
	}

	if (!sound_cb_get()) {
		LOG_ERR("DULT Sound: callbacks must be registered at this point");
		return -EINVAL;
	}

	is_enabled = true;

	return 0;
}

int dult_sound_reset(void)
{
	if (!is_enabled) {
		LOG_ERR("DULT Sound: already disabled");
		return -EALREADY;
	}

	is_enabled = false;
	sound_active = false;

	return 0;
}
