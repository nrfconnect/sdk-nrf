/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include <bluetooth/services/fast_pair/adv_manager.h>
#include <bluetooth/services/fast_pair/fast_pair.h>

#include "app_factory_reset.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

/** Factory reset state. */
enum app_factory_reset_state {
	/* Idle state. */
	APP_FACTORY_RESET_STATE_IDLE,

	/* Pending state. */
	APP_FACTORY_RESET_STATE_PENDING,

	/* Factory reset in progress state. */
	APP_FACTORY_RESET_STATE_IN_PROGRESS,
};

static void factory_reset_work_handle(struct k_work *w);

static K_WORK_DELAYABLE_DEFINE(factory_reset_work, factory_reset_work_handle);
static enum app_factory_reset_state factory_reset_state;
static bool factory_reset_ui_requested;

int bt_fast_pair_factory_reset_user_action_perform(void)
{
	int id;
	size_t id_count;
	uint8_t bt_id;

	/* Please be extra careful while implementing this hook function, as it can be
	 * executed in the context of the bt_fast_pair_factory_reset API call and the
	 * context of the bt_fast_pair_enable API call once the factory reset operation is
	 * interrupted by a system reboot. If this hook function returns an error, it
	 * will be propagated as a return value of the bt_fast_pair_enable API.
	 */
	LOG_INF("Factory Reset: resetting Bluetooth identity within the factory reset");

	/* Check if FP identity exists. */
	bt_id = bt_fast_pair_adv_manager_id_get();
	bt_id_get(NULL, &id_count);
	if (id_count > bt_id) {
		/* All non-volatile Bluetooth data linked to the Fast Pair Bluetooth identity
		 * (e.g Bluetooth bonds) will be removed after executing this action.
		 */
		id = bt_id_reset(bt_id, NULL, NULL);
		if (id != bt_id) {
			LOG_ERR("Factory Reset: bt_id_reset failed: err %d", id);
			return id;
		}
	} else {
		LOG_INF("Factory Reset: identity for factory reset does not exist");
	}

	return 0;
}

static void factory_reset_prepare(void)
{
	STRUCT_SECTION_FOREACH(app_factory_reset_callbacks, cbs) {
		if (cbs->prepare) {
			cbs->prepare();
		}
	}
}

static void factory_reset_executed(void)
{
	STRUCT_SECTION_FOREACH(app_factory_reset_callbacks, cbs) {
		if (cbs->executed) {
			cbs->executed();
		}
	}
}

static void factory_reset_perform(void)
{
	int err;
	bool fast_pair_is_ready = bt_fast_pair_is_ready();
	bool fp_adv_is_ready = bt_fast_pair_adv_manager_is_ready();

	LOG_INF("Performing reset to factory settings...");

	if (factory_reset_state == APP_FACTORY_RESET_STATE_IN_PROGRESS) {
		__ASSERT_NO_MSG(false);
		return;
	}

	factory_reset_state = APP_FACTORY_RESET_STATE_IN_PROGRESS;

	factory_reset_prepare();

	/* Disable the Fast Pair advertising module if it is active. */
	if (fp_adv_is_ready) {
		err = bt_fast_pair_adv_manager_disable();
		if (err) {
			LOG_ERR("Factory Reset: bt_fast_pair_adv_manager_disable "
				"failed (err %d)", err);
			goto finish;
		}
	}

	/* Disable the Fast Pair subsystem if it is active. */
	if (fast_pair_is_ready) {
		err = bt_fast_pair_disable();
		if (err) {
			LOG_ERR("Factory Reset: bt_fast_pair_disable failed: %d", err);
			goto finish;
		}
	}

	/* Perform a reset to Factory Settings. This operation will erase
	 * Fast Pair non-volatile data and FMDN specific non-volatile data.
	 */
	err = bt_fast_pair_factory_reset();
	if (err) {
		LOG_ERR("Factory Reset: bt_fast_pair_factory_reset failed: %d", err);
		goto finish;
	}

	/* Reenable the Fast Pair subsystem. */
	if (fast_pair_is_ready) {
		err = bt_fast_pair_enable();
		if (err) {
			LOG_ERR("Factory Reset: bt_fast_pair_enable failed: %d", err);
			goto finish;
		}
	}

	/* Reenable the Fast Pair advertising module if it was active before the reset. */
	if (fp_adv_is_ready) {
		err = bt_fast_pair_adv_manager_enable();
		if (err) {
			LOG_ERR("Factory Reset: bt_fast_pair_adv_manager_enable "
				"failed (err %d)", err);
			goto finish;
		}
	}

	LOG_INF("Reset to factory settings has completed");

	factory_reset_executed();

	factory_reset_state = APP_FACTORY_RESET_STATE_IDLE;

finish:
	if (err) {
		__ASSERT_NO_MSG(false);
		k_panic();
	}
}

static void factory_reset_work_handle(struct k_work *w)
{
	factory_reset_perform();
}

static void factory_reset_request_handle(enum app_ui_request request)
{
	if (request == APP_UI_REQUEST_FACTORY_RESET) {
		factory_reset_ui_requested = true;
	}
}

void app_factory_reset_schedule(k_timeout_t delay)
{
	if (factory_reset_state != APP_FACTORY_RESET_STATE_IDLE) {
		LOG_ERR("Factory Reset: rejecting scheduling operation");
		return;
	}

	(void) k_work_schedule(&factory_reset_work, delay);
	factory_reset_state = APP_FACTORY_RESET_STATE_PENDING;
}

void app_factory_reset_cancel(void)
{
	if (factory_reset_state == APP_FACTORY_RESET_STATE_IN_PROGRESS) {
		LOG_ERR("Factory Reset: rejecting cancelling operation");
		return;
	}

	(void) k_work_cancel_delayable(&factory_reset_work);
	factory_reset_state = APP_FACTORY_RESET_STATE_IDLE;
}

int app_factory_reset_init(void)
{
	__ASSERT_NO_MSG(factory_reset_state == APP_FACTORY_RESET_STATE_IDLE);

	if (factory_reset_ui_requested) {
		factory_reset_ui_requested = false;
		factory_reset_perform();
	}

	return 0;
}

APP_UI_REQUEST_LISTENER_REGISTER(factory_reset_request_handler, factory_reset_request_handle);
