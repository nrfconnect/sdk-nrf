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

#include "fp_adv_manager_use_case.h"

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

static void fp_adv_fmdn_provisioning_trigger_manage(bool enable)
{
	if (enable) {
		/* Fast Pair Implementation Guidelines for the locator tag use case:
		 * after the Provider was paired, it should not change its MAC address
		 * till FMDN is provisioned or till 5 minutes passes.
		 */
		(void) k_work_schedule(&fp_adv_rpa_suspension_work,
				       K_MINUTES(FP_ADV_RPA_SUSPENSION_TIMEOUT));
	} else {
		(void) k_work_cancel_delayable(&fp_adv_rpa_suspension_work);
	}

	bt_fast_pair_adv_manager_request(
		&bt_fast_pair_adv_manager_trigger_fmdn_provisioning, enable);
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
	LOG_DBG("Fast Pair Adv Manager: Locator Tag: clock information synchronized with "
		"the authenticated Bluetooth peer");

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

static void fp_adv_provisioning_state_changed(bool provisioned)
{
	fmdn_provisioned = provisioned;

	if (!bt_fast_pair_adv_manager_is_ready()) {
		return;
	}

	if (provisioned) {
		fp_adv_fmdn_provisioning_trigger_manage(false);
	} else {
		fp_adv_clock_sync_trigger_manage(false);
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
		fp_adv_fmdn_provisioning_trigger_manage(true);
	}

	fp_account_key_present = bt_fast_pair_has_account_key();
}

static struct bt_fast_pair_info_cb fp_info_callbacks = {
	.account_key_written = fp_adv_account_key_written,
};

static bool fp_adv_is_first_enable_after_bootup(void)
{
	static bool first_enable_after_bootup = true;
	bool is_first_enable_after_bootup = first_enable_after_bootup;

	first_enable_after_bootup = false;

	return is_first_enable_after_bootup;
}

static int fp_adv_manager_locator_tag_enable_initial(void)
{
	/* Handle the clock synchronization trigger. */
	if (fmdn_provisioned) {
		fp_adv_clock_sync_trigger_manage(true);
	}

	return 0;
}

static int fp_adv_manager_locator_tag_enable_subsequent(void)
{
	/* Handle the clock synchronization trigger. */
	if (!fmdn_provisioned) {
		fp_adv_clock_sync_trigger_manage(false);
	} else {
		/* Left empty intentionally: this state is used to recreate the trigger state
		 * after the bt_fast_pair_adv_manager_disable operation.
		 */
	}

	/* Handle the FMDN provisioning trigger. */
	if (fp_account_key_present && !fmdn_provisioned) {
		/* The FMDN provisioning trigger is inactive as the Fast Pair Advertising Manager
		 * gets disabled (bt_fast_pair_adv_manager_disable) by the user during the
		 * provisioning procedure.
		 */
		LOG_WRN("Fast Pair Adv Manager: Locator Tag: FMDN provisioning trigger inactive");
	}

	return 0;
}

int fp_adv_manager_locator_tag_enable(void)
{
	bool is_first_enable_after_bootup = fp_adv_is_first_enable_after_bootup();

	fp_account_key_present = bt_fast_pair_has_account_key();
	fmdn_provisioned = bt_fast_pair_fmdn_is_provisioned();

	if (is_first_enable_after_bootup) {
		return fp_adv_manager_locator_tag_enable_initial();
	} else {
		return fp_adv_manager_locator_tag_enable_subsequent();
	}
}

int fp_adv_manager_locator_tag_disable(void)
{
	(void) k_work_cancel_delayable(&fp_adv_rpa_suspension_work);

	/* Disable the FMDN provisioning trigger. The trigger state will not be restored
	 * during the enable operation as the FMDN provisioning is time-based operation.
	 * The Provider should typically advertise continuously for 5 minutes until it is
	 * factory reset or successfully provisioned.
	 */
	fp_adv_fmdn_provisioning_trigger_manage(false);

	return 0;
}

static const struct fp_adv_manager_use_case_cb use_case_cb = {
	.enable = fp_adv_manager_locator_tag_enable,
	.disable = fp_adv_manager_locator_tag_disable,
};

int fp_adv_manager_locator_tag_init(void)
{
	int err;

	err = bt_fast_pair_fmdn_info_cb_register(&fmdn_info_cb);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: Locator Tag: "
			"bt_fast_pair_fmdn_info_cb_register returned error: %d", err);
		return err;
	}

	err = bt_fast_pair_info_cb_register(&fp_info_callbacks);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: Locator Tag: "
			"bt_fast_pair_info_cb_register failed (err %d)", err);
		return err;
	}

	fp_adv_manager_use_case_cb_register(&use_case_cb);

	return 0;
}

SYS_INIT(fp_adv_manager_locator_tag_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
