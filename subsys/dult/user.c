/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <errno.h>

#include <dult.h>
#include "dult_battery.h"
#include "dult_bt_anos.h"
#include "dult_id.h"
#include "dult_near_owner_state.h"
#include "dult_sound.h"
#include "dult_user.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_user, CONFIG_DULT_LOG_LEVEL);

static atomic_ptr_t cur_user;
static atomic_t is_enabled;

int dult_user_register(const struct dult_user *user)
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

	if (!(user->accessory_capabilities & BIT(DULT_ACCESSORY_CAPABILITY_PLAY_SOUND_BIT_POS))) {
		return -EINVAL;
	}

	if (!(user->accessory_capabilities &
	      BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_NFC_BIT_POS)) &&
	    !(user->accessory_capabilities &
	      BIT(DULT_ACCESSORY_CAPABILITY_ID_LOOKUP_BLE_BIT_POS))) {
		return -EINVAL;
	}

	if (!atomic_ptr_cas(&cur_user, NULL, (atomic_ptr_val_t)user)) {
		LOG_ERR("DULT user already registered");
		return -EALREADY;
	}

	return 0;
}

bool dult_user_is_registered(const struct dult_user *user)
{
	if (!user) {
		return false;
	}

	if (atomic_ptr_get(&cur_user) != user) {
		return false;
	}

	return true;
}

int dult_enable(const struct dult_user *user)
{
	int err;

	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (atomic_get(&is_enabled)) {
		LOG_ERR("DULT already enabled");
		return -EALREADY;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (IS_ENABLED(CONFIG_DULT_BATTERY)) {
		err = dult_battery_enable();
		if (err) {
			LOG_ERR("dult_battery_enable returned an error: %d", err);
			return err;
		}
	}

	err = dult_bt_anos_enable();
	if (err) {
		LOG_ERR("dult_bt_anos_enable returned an error: %d", err);
		return err;
	}

	err = dult_sound_enable();
	if (err) {
		LOG_ERR("dult_sound_enable returned an error: %d", err);
		return err;
	}

	err = dult_id_enable();
	if (err) {
		LOG_ERR("dult_id_enable returned an error: %d", err);
		return err;
	}

	atomic_set(&is_enabled, true);

	LOG_INF("DULT enabled");

	return 0;
}

int dult_reset(const struct dult_user *user)
{
	int err;

	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	dult_near_owner_state_reset();

	err = dult_id_reset();
	if (err) {
		LOG_ERR("dult_id_reset returned an error: %d", err);
		return err;
	}

	err = dult_sound_reset();
	if (err) {
		LOG_ERR("dult_sound_reset returned an error: %d", err);
		return err;
	}

	err = dult_bt_anos_reset();
	if (err) {
		LOG_ERR("dult_bt_anos_reset returned an error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_DULT_BATTERY)) {
		err = dult_battery_reset();
		if (err) {
			LOG_ERR("dult_battery_reset returned an error: %d", err);
			return err;
		}
	}

	atomic_set(&is_enabled, false);
	atomic_ptr_set(&cur_user, NULL);

	LOG_INF("DULT reset completed");

	return 0;
}

const struct dult_user *dult_user_get(void)
{
	return atomic_ptr_get(&cur_user);
}

bool dult_user_is_ready(void)
{
	return atomic_get(&is_enabled);
}
