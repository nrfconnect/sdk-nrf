/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <errno.h>

#include "dult_bt_anos.h"
#include "dult_user.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_sound, CONFIG_DULT_LOG_LEVEL);

static bool is_enabled;
static bool sound_active;
static enum dult_sound_src sound_src;
static const struct dult_sound_cb *sound_cb;

int dult_sound_cb_register(const struct dult_user *user, const struct dult_sound_cb *cb)
{
	if (dult_user_is_ready()) {
		LOG_ERR("DULT Sound: module must be disabled to register callbacks");
		return -EACCES;
	}
	__ASSERT_NO_MSG(!is_enabled);

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (sound_cb) {
		LOG_ERR("DULT Sound: sound callbacks already registered");
		return -EALREADY;
	}

	if (!cb || !cb->sound_start || !cb->sound_stop) {
		return -EINVAL;
	}

	sound_cb = cb;

	return 0;
}

int dult_sound_state_update(const struct dult_user *user,
			    const struct dult_sound_state_param *param)
{
	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!dult_user_is_ready()) {
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

	return 0;
}

static void anos_sound_start(void)
{
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
	__ASSERT(sound_cb && sound_cb->sound_stop,
		 "DULT Sound: stop callback is not populated");

	sound_cb->sound_stop(DULT_SOUND_SRC_BT_GATT);
}

const static struct dult_bt_anos_sound_cb anos_sound_cb = {
	.sound_start = anos_sound_start,
	.sound_stop = anos_sound_stop,
};

int dult_sound_enable(void)
{
	static bool bt_anos_sound_cb_registered;

	if (!bt_anos_sound_cb_registered) {
		dult_bt_anos_sound_cb_register(&anos_sound_cb);
		bt_anos_sound_cb_registered = true;
	}

	if (is_enabled) {
		LOG_ERR("DULT Sound: already enabled");
		return -EALREADY;
	}

	if (!sound_cb) {
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
	sound_cb = NULL;
	sound_active = false;

	return 0;
}
