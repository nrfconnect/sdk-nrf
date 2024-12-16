/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <bluetooth/services/fast_pair/adv_manager.h>
#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

#include "fp_adv_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair_adv_manager);

/* RPA suspension timeout in minutes for the Fast Pair advertising set
 * as recommended by the FMDN specification.
 */
#define FP_ADV_RPA_SUSPENSION_TIMEOUT (5)

/* Used to wait for the FMDN provisioning procedure after the Account Key write. */
BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER(
	bt_fast_pair_adv_manager_trigger_fmdn_provisioning,
	"fmdn_provisioning",
	(&(const struct bt_fast_pair_adv_manager_trigger_config) {
		.suspend_rpa = true,
	}));

/* Used to trigger the clock synchronization after the bootup if provisioned. */
BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER(
	bt_fast_pair_adv_manager_trigger_clock_sync,
	"clock_sync",
	NULL);

static void fp_adv_rpa_suspension_work_handle(struct k_work *w);

static K_WORK_DELAYABLE_DEFINE(fp_adv_rpa_suspension_work, fp_adv_rpa_suspension_work_handle);

static bool fmdn_provisioned;
static bool fp_account_key_present;

static void fp_adv_rpa_suspension_work_handle(struct k_work *w)
{
	bt_fast_pair_adv_manager_request(
		&bt_fast_pair_adv_manager_trigger_fmdn_provisioning, false);
}

static void fp_adv_clock_sync_trigger_manage(bool enable)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG_CLOCK_SYNC_TRIGGER)) {
		return;
	}

	bt_fast_pair_adv_manager_request(&bt_fast_pair_adv_manager_trigger_clock_sync, enable);
}

static void fp_adv_clock_synced(void)
{
	LOG_INF("Fast Pair Adv Manager:: clock information synchronized with the authenticated "
		"Bluetooth peer");

	if (fmdn_provisioned) {
		/* Fast Pair Implementation Guidelines for the locator tag use case:
		 * After a power loss, the device should advertise non-discoverable
		 * Fast Pair frames until the next invocation of read beacon parameters.
		 * This lets the Seeker detect the device and synchronize the clock even
		 * if a significant clock drift occurred.
		 */
		fp_adv_clock_sync_trigger_manage(false);
	}
}

static bool fmdn_provisioning_state_is_first_cb_after_bootup(void)
{
	static bool first_cb_after_bootup = true;
	bool is_first_cb_after_bootup = first_cb_after_bootup;

	first_cb_after_bootup = false;

	return is_first_cb_after_bootup;
}

static void fp_adv_provisioning_state_changed(bool provisioned)
{
	bool is_first_cb_after_bootup = fmdn_provisioning_state_is_first_cb_after_bootup();

	fp_adv_clock_sync_trigger_manage(is_first_cb_after_bootup && provisioned);

	fmdn_provisioned = provisioned;

	if (!bt_fast_pair_adv_manager_is_ready()) {
		return;
	}

	if (provisioned) {
		(void) k_work_cancel_delayable(&fp_adv_rpa_suspension_work);
		bt_fast_pair_adv_manager_request(
			&bt_fast_pair_adv_manager_trigger_fmdn_provisioning, false);
	}
}

static struct bt_fast_pair_fmdn_info_cb fmdn_info_cb = {
	.clock_synced = fp_adv_clock_synced,
	.provisioning_state_changed = fp_adv_provisioning_state_changed,
};

static void fp_adv_account_key_written(struct bt_conn *conn)
{
	/* The first and only Account Key write starts the FMDN provisioning. */
	if (!fmdn_provisioned && !fp_account_key_present &&
	    bt_fast_pair_adv_manager_is_ready()) {
		/* Fast Pair Implementation Guidelines for the locator tag use case:
		 * after the Provider was paired, it should not change its MAC address
		 * till FMDN is provisioned or till 5 minutes passes.
		 */
		bt_fast_pair_adv_manager_request(
			&bt_fast_pair_adv_manager_trigger_fmdn_provisioning, true);
		(void) k_work_schedule(&fp_adv_rpa_suspension_work,
				       K_MINUTES(FP_ADV_RPA_SUSPENSION_TIMEOUT));
	}

	fp_account_key_present = bt_fast_pair_has_account_key();
}

static struct bt_fast_pair_info_cb fp_info_callbacks = {
	.account_key_written = fp_adv_account_key_written,
};

int fp_adv_manager_locator_tag_enable(void)
{
	fp_account_key_present = bt_fast_pair_has_account_key();

	return 0;
}

int fp_adv_manager_locator_tag_disable(void)
{
	(void) k_work_cancel_delayable(&fp_adv_rpa_suspension_work);

	/* Disable the active triggers. The trigger state won't be restored during the enable
	 * operation as the disable operation reset their state.
	 */
	fp_adv_clock_sync_trigger_manage(false);
	bt_fast_pair_adv_manager_request(
		&bt_fast_pair_adv_manager_trigger_fmdn_provisioning, false);

	return 0;
}

static const struct fp_adv_manager_use_case_cb use_case_cb = {
	.enable = fp_adv_manager_locator_tag_enable,
};

int fp_adv_manager_locator_tag_init(void)
{
	int err;

	err = bt_fast_pair_fmdn_info_cb_register(&fmdn_info_cb);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: "
			"bt_fast_pair_fmdn_info_cb_register returned error: %d", err);
		return err;
	}

	err = bt_fast_pair_info_cb_register(&fp_info_callbacks);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: bt_fast_pair_info_cb_register failed (err %d)",
			err);
		return err;
	}

	fp_adv_manager_use_case_cb_register(&use_case_cb);

	return 0;
}

SYS_INIT(fp_adv_manager_locator_tag_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
