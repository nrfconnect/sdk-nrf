/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_battery.h"
#include "app_factory_reset.h"
#include "app_fp_adv.h"
#include "app_ring.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn, LOG_LEVEL_INF);

/* Bluetooth identity used by the Fast Pair and FMDN modules.*/
#define APP_BT_ID 1

/* Semaphore timeout in seconds. */
#define INIT_SEM_TIMEOUT (60)

/* Factory reset delay in seconds since the trigger operation. */
#define FACTORY_RESET_DELAY (3)

/* FMDN provisioning timeout in minutes as recommended by the specification. */
#define FMDN_PROVISIONING_TIMEOUT (5)

/* FMDN recovery mode timeout in minutes. */
#define FMDN_RECOVERY_MODE_TIMEOUT (CONFIG_BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY_TIMEOUT)

/* FMDN identification mode timeout in minutes. */
#define FMDN_ID_MODE_TIMEOUT (CONFIG_DULT_ID_READ_STATE_TIMEOUT)

/* Minimum button hold time in milliseconds to trigger the FMDN recovery mode. */
#define FMDN_RECOVERY_MODE_BTN_MIN_HOLD_TIME_MS (3000)

/* FMDN advertising interval 2s (0x0C80 in hex). */
#define FMDN_ADV_INTERVAL (0x0C80)

/* Factory reset reason. */
enum factory_reset_trigger {
	FACTORY_RESET_TRIGGER_NONE,
	FACTORY_RESET_TRIGGER_KEY_STATE_MISMATCH,
	FACTORY_RESET_TRIGGER_PROVISIONING_TIMEOUT,
};

static bool fmdn_provisioned;
static bool fmdn_recovery_mode;
static bool fmdn_id_mode;
static bool fp_account_key_present;

static bool factory_reset_executed;
static enum factory_reset_trigger factory_reset_trigger;

static void init_work_handle(struct k_work *w);

static K_SEM_DEFINE(init_work_sem, 0, 1);
static K_WORK_DEFINE(init_work, init_work_handle);

static void fmdn_factory_reset_executed(void)
{
	/* Clear the trigger state for the scheduled factory reset operations. */
	factory_reset_trigger = FACTORY_RESET_TRIGGER_NONE;
	factory_reset_executed = true;
}

static void fmdn_factory_reset_schedule(enum factory_reset_trigger trigger, k_timeout_t delay)
{
	app_factory_reset_schedule(delay, fmdn_factory_reset_executed);
	factory_reset_trigger = trigger;
}

static void fmdn_factory_reset_cancel(void)
{
	app_factory_reset_cancel();
	factory_reset_trigger = FACTORY_RESET_TRIGGER_NONE;
}

static enum bt_security_err pairing_accept(struct bt_conn *conn,
					   const struct bt_conn_pairing_feat *const feat)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(feat);

	/* Fast Pair Implementation Guidelines for the locator tag use case:
	 * Provider should reject normal Bluetooth pairing attempts. It should
	 * only accept Fast Pair pairing.
	 */

	LOG_WRN("Normal Bluetooth pairing not allowed");

	return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
}

static const struct bt_conn_auth_cb conn_auth_callbacks = {
	.pairing_accept = pairing_accept,
};

static bool identifying_info_read_allow(struct bt_conn *conn)
{
	ARG_UNUSED(conn);

	if (!fmdn_provisioned) {
		return true;
	}

	if (fmdn_id_mode) {
		return true;
	}

	LOG_INF("Rejecting read operation of identifying information");

	return false;
}

static bool gatt_read_authorize(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	const struct bt_uuid *uuid_block_list[] = {
		/* GAP service characteristics */
		BT_UUID_GAP_DEVICE_NAME,
	};

	for (size_t i = 0; i < ARRAY_SIZE(uuid_block_list); i++) {
		if (bt_uuid_cmp(attr->uuid, uuid_block_list[i]) == 0) {
			/* Fast Pair Implementation Guidelines for the locator tag use case:
			 * The Provider shouldn't expose any identifying information
			 * in an unauthenticated manner (e.g. names or identifiers).
			 */
			return identifying_info_read_allow(conn);
		}
	}

	return true;
}

static const struct bt_gatt_authorization_cb gatt_authorization_callbacks = {
	.read_authorize = gatt_read_authorize,
};

static void fp_account_key_written(struct bt_conn *conn)
{
	LOG_INF("Fast Pair: Account Key write");

	app_fp_adv_mode_set(APP_FP_ADV_MODE_NOT_DISCOVERABLE);

	/* Fast Pair Implementation Guidelines for the locator tag use case:
	 * trigger the reset to factory settings if there is no FMDN
	 * provisioning operation within 5 minutes.
	 */
	if (!fp_account_key_present) {
		fmdn_factory_reset_schedule(
			FACTORY_RESET_TRIGGER_PROVISIONING_TIMEOUT,
			K_MINUTES(FMDN_PROVISIONING_TIMEOUT));

		/* Fast Pair Implementation Guidelines for the locator tag use case:
		 * after the Provider was paired, it should not change its MAC address
		 * till FMDN is provisioned or till 5 minutes passes.
		 */
		app_fp_adv_rpa_rotation_suspend(true);
	}

	fp_account_key_present = bt_fast_pair_has_account_key();
}

static const struct bt_fast_pair_info_cb fp_info_callbacks = {
	.account_key_written = fp_account_key_written,
};

static void fmdn_recovery_mode_exited(void)
{
	LOG_INF("FMDN: recovery mode exited");

	fmdn_recovery_mode = false;
	app_ui_state_change_indicate(APP_UI_STATE_RECOVERY_MODE, fmdn_recovery_mode);
}

static void fmdn_id_mode_exited(void)
{
	LOG_INF("FMDN: identification mode exited");

	fmdn_id_mode = false;
	app_ui_state_change_indicate(APP_UI_STATE_ID_MODE, fmdn_id_mode);
}

static void fmdn_read_mode_exited(enum bt_fast_pair_fmdn_read_mode mode)
{
	switch (mode) {
	case BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY:
		fmdn_recovery_mode_exited();
		break;
	case BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID:
		fmdn_id_mode_exited();
		break;
	default:
		/* Unsupported mode: should not happen. */
		__ASSERT_NO_MSG(0);
		break;
	}
}

static const struct bt_fast_pair_fmdn_read_mode_cb fmdn_read_mode_cb = {
	.exited = fmdn_read_mode_exited,
};

static void fmdn_recovery_mode_action_handle(void)
{
	int err;

	if (!fmdn_provisioned) {
		LOG_INF("FMDN: the recovery mode is not available in the "
			"unprovisioned state");
		return;
	}

	if (fmdn_recovery_mode) {
		LOG_INF("FMDN: refreshing the recovery mode timeout");
	} else {
		LOG_INF("FMDN: entering the recovery mode for %d minute(s)",
			FMDN_RECOVERY_MODE_TIMEOUT);
	}

	err = bt_fast_pair_fmdn_read_mode_enter(BT_FAST_PAIR_FMDN_READ_MODE_FMDN_RECOVERY);
	if (err) {
		LOG_ERR("FMDN: failed to enter the recovery mode: err=%d", err);
		return;
	}

	fmdn_recovery_mode = true;
	app_ui_state_change_indicate(APP_UI_STATE_RECOVERY_MODE, fmdn_recovery_mode);
}

static void fmdn_id_mode_action_handle(void)
{
	int err;

	if (!fmdn_provisioned) {
		LOG_INF("FMDN: the identification mode is not available in the "
			"unprovisioned state. "
			"Identifying info can always be read in this state.");
		return;
	}

	if (fmdn_id_mode) {
		LOG_INF("FMDN: refreshing the identification mode timeout");
	} else {
		LOG_INF("FMDN: entering the identification mode for %d minute(s)",
			FMDN_ID_MODE_TIMEOUT);
	}

	err = bt_fast_pair_fmdn_read_mode_enter(BT_FAST_PAIR_FMDN_READ_MODE_DULT_ID);
	if (err) {
		LOG_ERR("FMDN: failed to enter the identification mode: err=%d", err);
		return;
	}

	/* Fast Pair Implementation Guidelines for the locator tag use case:
	 * The Provider shouldn't expose any identifying information
	 * in an unauthenticated manner (e.g. names or identifiers).
	 *
	 * The DULT identification mode is also used to allow reading of Bluetooth
	 * characteristics with identifying information for a limited time in the
	 * provisioned state.
	 */
	fmdn_id_mode = true;
	app_ui_state_change_indicate(APP_UI_STATE_ID_MODE, fmdn_id_mode);
}

static void fmdn_mode_request_handle(enum app_ui_request request)
{
	/* It is assumed that the callback executes in the cooperative
	 * thread context as it interacts with the FMDN API.
	 */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (request == APP_UI_REQUEST_RECOVERY_MODE_ENTER) {
		fmdn_recovery_mode_action_handle();
	} else if (request == APP_UI_REQUEST_ID_MODE_ENTER) {
		fmdn_id_mode_action_handle();
	}
}

static void fmdn_clock_synced(void)
{
	LOG_INF("FMDN: clock information synchronized with the authenticated Bluetooth peer");

	if (fmdn_provisioned) {
		/* Fast Pair Implementation Guidelines for the locator tag use case:
		 * After a power loss, the device should advertise non-discoverable
		 * Fast Pair frames until the next invocation of read beacon parameters.
		 * This lets the Seeker detect the device and synchronize the clock even
		 * if a significant clock drift occurred.
		 */
		app_fp_adv_mode_set(APP_FP_ADV_MODE_OFF);
	}
}

static void fmdn_provisioning_state_changed(bool provisioned)
{
	static bool is_first_state_changed_cb = true;

	enum app_fp_adv_mode fp_adv_mode;

	LOG_INF("FMDN: state changed to %s",
		provisioned ? "provisioned" : "unprovisioned");

	app_ui_state_change_indicate(APP_UI_STATE_PROVISIONED, provisioned);
	fmdn_provisioned = provisioned;

	/* Fast Pair Implementation Guidelines for the locator tag use case:
	 * cancel the provisioning timeout.
	 */
	if (provisioned &&
	    (factory_reset_trigger == FACTORY_RESET_TRIGGER_PROVISIONING_TIMEOUT)) {
		fmdn_factory_reset_cancel();
		app_fp_adv_rpa_rotation_suspend(false);
	}

	/* Fast Pair Implementation Guidelines for the locator tag use case:
	 * trigger the reset to factory settings on the unprovisioning operation
	 * or on the loss of the Owner Account Key.
	 */
	fp_account_key_present = bt_fast_pair_has_account_key();
	if (fp_account_key_present != provisioned) {
		app_fp_adv_mode_set(APP_FP_ADV_MODE_OFF);

		/* Delay the factory reset operation to allow the local device
		 * to send a response to the unprovisioning command and give
		 * the connected peer necessary time to finalize its operations
		 * and shutdown the connection.
		 */
		fmdn_factory_reset_schedule(
			FACTORY_RESET_TRIGGER_KEY_STATE_MISMATCH,
			K_SECONDS(FACTORY_RESET_DELAY));

		return;
	}

	/* Triggered on the unprovisioning operation. */
	if (factory_reset_executed) {
		LOG_INF("Please press a button to put the device in the Fast Pair discoverable "
			"advertising mode after a reset to factory settings");
		factory_reset_executed = false;

		return;
	}

	/* Select the Fast Pair advertising mode according to the FMDN provisioning state. */
	if (provisioned) {
		if (is_first_state_changed_cb) {
			fp_adv_mode = APP_FP_ADV_MODE_NOT_DISCOVERABLE;
		} else {
			fp_adv_mode = APP_FP_ADV_MODE_OFF;
		}
	} else {
		fp_adv_mode = APP_FP_ADV_MODE_DISCOVERABLE;
	}
	app_fp_adv_mode_set(fp_adv_mode);

	is_first_state_changed_cb = false;
}

static struct bt_fast_pair_fmdn_info_cb fmdn_info_cb = {
	.clock_synced = fmdn_clock_synced,
	.provisioning_state_changed = fmdn_provisioning_state_changed,
};

static int fast_pair_prepare(void)
{
	int err;

	err = app_fp_adv_id_set(APP_BT_ID);
	if (err) {
		LOG_ERR("Fast Pair: app_fp_adv_id_set failed (err %d)", err);
		return err;
	}

	err = app_fp_adv_init();
	if (err) {
		LOG_ERR("Fast Pair: app_fp_adv_init failed (err %d)", err);
		return err;
	}

	return 0;
}

static int fmdn_prepare(void)
{
	int err;
	const struct bt_fast_pair_fmdn_adv_param fmdn_adv_param =
		BT_FAST_PAIR_FMDN_ADV_PARAM_INIT(
			FMDN_ADV_INTERVAL, FMDN_ADV_INTERVAL);

	err = bt_fast_pair_fmdn_id_set(APP_BT_ID);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_id_set failed (err %d)", err);
		return err;
	}

	/* Application configuration of the advertising interval is equal to
	 * the default value that is defined in the FMDN module. This API
	 * call is only for demonstration purposes.
	 */
	err = bt_fast_pair_fmdn_adv_param_set(&fmdn_adv_param);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_adv_param_set failed (err %d)", err);
		return err;
	}

	err = bt_fast_pair_fmdn_info_cb_register(&fmdn_info_cb);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_info_cb_register failed (err %d)", err);
		return err;
	}

	err = bt_fast_pair_fmdn_read_mode_cb_register(&fmdn_read_mode_cb);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_read_mode_cb_register failed (err %d)", err);
		return err;
	}

	err = bt_fast_pair_info_cb_register(&fp_info_callbacks);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_info_cb_register failed (err %d)", err);
		return err;
	}

	return 0;
}

static int app_id_create(void)
{
	int ret;
	size_t count;

	BUILD_ASSERT(CONFIG_BT_ID_MAX > APP_BT_ID);

	/* Check if application identity wasn't already created. */
	bt_id_get(NULL, &count);
	if (count > APP_BT_ID) {
		return 0;
	}

	/* Create the application identity. */
	do {
		ret = bt_id_create(NULL, NULL);
		if (ret < 0) {
			return ret;
		}
	} while (ret != APP_BT_ID);

	return 0;
}

static void init_work_handle(struct k_work *w)
{
	int err;

	err = app_ui_init();
	if (err) {
		LOG_ERR("UI module init failed (err %d)", err);
		return;
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Registering authentication callbacks failed (err %d)", err);
		return;
	}

	err = bt_gatt_authorization_cb_register(&gatt_authorization_callbacks);
	if (err) {
		LOG_ERR("Registering GATT authorization callbacks failed (err %d)", err);
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized");

	err = settings_load();
	if (err) {
		LOG_ERR("Settings load failed (err: %d)", err);
		return;
	}
	LOG_INF("Settings loaded");

	err = app_id_create();
	if (err) {
		LOG_ERR("Application identity failed to create (err %d)", err);
		return;
	}

	err = app_battery_init();
	if (err) {
		LOG_ERR("FMDN: app_battery_init failed (err %d)", err);
		return;
	}

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)) {
		err = app_ring_init();
		if (err) {
			LOG_ERR("FMDN: app_ring_init failed (err %d)", err);
			return;
		}
	}

	err = fast_pair_prepare();
	if (err) {
		LOG_ERR("FMDN: fast_pair_prepare failed (err %d)", err);
		return;
	}

	err = fmdn_prepare();
	if (err) {
		LOG_ERR("FMDN: fmdn_prepare failed (err %d)", err);
		return;
	}

	err = app_factory_reset_init();
	if (err) {
		LOG_ERR("FMDN: app_factory_reset_init failed (err %d)", err);
		return;
	}

	err = bt_fast_pair_enable();
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_enable failed (err %d)", err);
		return;
	}

	k_sem_give(&init_work_sem);
}

int main(void)
{
	int err;

	LOG_INF("Starting Bluetooth Fast Pair locator tag example");

	/* Switch to the cooperative thread context before interaction
	 * with the Fast Pair and FMDN API.
	 */
	(void) k_work_submit(&init_work);
	err = k_sem_take(&init_work_sem, K_SECONDS(INIT_SEM_TIMEOUT));
	if (err) {
		k_panic();
		return 0;
	}

	LOG_INF("Sample has started");

	app_ui_state_change_indicate(APP_UI_STATE_APP_RUNNING, true);

	return 0;
}

APP_UI_REQUEST_LISTENER_REGISTER(fmdn_mode_request_handler, fmdn_mode_request_handle);
