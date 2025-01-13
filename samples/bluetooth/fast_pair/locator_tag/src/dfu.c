/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_version.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/adv_prov.h>

#include <bluetooth/services/fast_pair/adv_manager.h>
#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#include "app_dfu.h"
#include "app_factory_reset.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

/* DFU mode timeout in minutes. */
#define DFU_MODE_TIMEOUT_MIN (5)

static const struct bt_data data = BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL);
static bool dfu_mode;

BT_FAST_PAIR_ADV_MANAGER_TRIGGER_REGISTER(
	fp_adv_trigger_dfu,
	"dfu",
	(&(const struct bt_fast_pair_adv_manager_trigger_config) {
		.suspend_rpa = true,
	}));

static void dfu_mode_timeout_work_handle(struct k_work *w);
static K_WORK_DELAYABLE_DEFINE(dfu_mode_timeout_work, dfu_mode_timeout_work_handle);

BUILD_ASSERT((APP_VERSION_MAJOR == CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MAJOR) &&
	     (APP_VERSION_MINOR == CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_MINOR) &&
	     (APP_PATCHLEVEL == CONFIG_BT_FAST_PAIR_FMDN_DULT_FIRMWARE_VERSION_REVISION),
	     "Firmware version mismatch. Update the DULT FW version in the Kconfig file "
	     "to be aligned with the VERSION file.");

static void dfu_mode_change(bool new_mode)
{
	if (dfu_mode == new_mode) {
		return;
	}

	LOG_INF("DFU: mode %sabled", new_mode ? "en" : "dis");

	dfu_mode = new_mode;

	bt_fast_pair_adv_manager_request(&fp_adv_trigger_dfu, dfu_mode);
	bt_fast_pair_adv_manager_payload_refresh();

	app_ui_state_change_indicate(APP_UI_STATE_DFU_MODE, dfu_mode);
}

static void dfu_factory_reset_prepare(void)
{
	dfu_mode_change(false);
}

APP_FACTORY_RESET_CALLBACKS_REGISTER(factory_reset_cbs, dfu_factory_reset_prepare, NULL);

bool app_dfu_bt_gatt_operation_allow(const struct bt_uuid *uuid)
{
	if (bt_uuid_cmp(uuid, SMP_BT_CHR_UUID) != 0) {
		return true;
	}

	if (!dfu_mode) {
		LOG_WRN("DFU: SMP characteristic access denied, DFU mode is not active");
		return false;
	}

	(void) k_work_reschedule(&dfu_mode_timeout_work, K_MINUTES(DFU_MODE_TIMEOUT_MIN));

	return true;
}

/* Due to using legacy advertising set size, add SMP UUID to either AD or SD,
 * depending on the space availability related to the advertising mode.
 * Otherwise, the advertising set size would be exceeded and the advertising
 * would not start.
 * The SMP UUID can be added only to one of the data sets.
 */

static int get_ad_data(struct bt_data *ad, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(state);
	ARG_UNUSED(fb);

	if (!dfu_mode) {
		return -ENOENT;
	}

	if (!bt_fast_pair_adv_manager_is_pairing_mode()) {
		return -ENOENT;
	}

	*ad = data;

	return 0;
}

static int get_sd_data(struct bt_data *sd, const struct bt_le_adv_prov_adv_state *state,
		    struct bt_le_adv_prov_feedback *fb)
{
	ARG_UNUSED(state);
	ARG_UNUSED(fb);

	if (!dfu_mode) {
		return -ENOENT;
	}

	if (bt_fast_pair_adv_manager_is_pairing_mode()) {
		return -ENOENT;
	}

	*sd = data;

	return 0;
}

/* Used in discoverable adv */
BT_LE_ADV_PROV_AD_PROVIDER_REGISTER(smp_ad, get_ad_data);

/* Used in the not-discoverable adv */
BT_LE_ADV_PROV_SD_PROVIDER_REGISTER(smp_sd, get_sd_data);

static void dfu_mode_action_handle(void)
{
	if (dfu_mode) {
		LOG_INF("DFU: refreshing the DFU mode timeout");
	} else {
		LOG_INF("DFU: entering the DFU mode for %d minute(s)",
			DFU_MODE_TIMEOUT_MIN);
	}

	(void) k_work_reschedule(&dfu_mode_timeout_work, K_MINUTES(DFU_MODE_TIMEOUT_MIN));

	dfu_mode_change(true);
}

static void dfu_mode_timeout_work_handle(struct k_work *w)
{
	LOG_INF("DFU: timeout expired");

	dfu_mode_change(false);
}

void app_dfu_fw_version_log(void)
{
	LOG_INF("DFU: Firmware version: %s", APP_VERSION_TWEAK_STRING);
}

static void dfu_mode_request_handle(enum app_ui_request request)
{
	/* It is assumed that the callback executes in the cooperative
	 * thread context as it interacts with the FMDN API.
	 */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (request == APP_UI_REQUEST_DFU_MODE_ENTER) {
		dfu_mode_action_handle();
	}
}

APP_UI_REQUEST_LISTENER_REGISTER(dfu_mode_request_handler, dfu_mode_request_handle);
