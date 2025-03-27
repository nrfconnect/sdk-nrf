/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <suit_manifest_variables.h>
#include <device_management.h>
#include <zephyr/logging/log.h>
#include <sdfw/sdfw_services/suit_service.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <mpsl/flash_sync_rpc_host.h>

LOG_MODULE_DECLARE(AB, CONFIG_SUIT_LOG_LEVEL);

#define NOT_CONFIRMED 2
#define CONFIRMED     3

#define BOOT_PREFERENCE_A 1
#define BOOT_PREFERENCE_B 2

/** @brief Radio firmware self test
 *
 * @details
 * End-device specific self test should be implemented here.
 */
static bool radio_domain_healthy(void)
{
	return bt_is_ready();
}

/** @brief Application firmware self test
 *
 * @details
 * End-device specific self test should be implemented here. Enabling
 * CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE allows to emulate a faulty
 * firmware, unable to confirm its health, and ultimately to test
 * a rollback to previous firmware after the update.
 */
static bool app_domain_healthy(void)
{
	if (IS_ENABLED(CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE)) {
		return false;
	}

	return true;
}

bool device_is_in_recovery_mode(void)
{
	static suit_boot_mode_t mode = SUIT_BOOT_MODE_UNKNOWN;
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	if (mode == SUIT_BOOT_MODE_UNKNOWN) {
		err = suit_boot_mode_read(&mode);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Cannot get the boot mode");
			mode = SUIT_BOOT_MODE_FAIL_STARTUP;
		}
	}

	return (mode == SUIT_BOOT_MODE_POST_INVOKE_RECOVERY);
}

uint32_t device_boot_status_get(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	static uint32_t mfst_var_boot_status;

	if (mfst_var_boot_status == 0) {
		err = suit_mfst_var_get(CONFIG_ID_VAR_BOOT_STATUS, &mfst_var_boot_status);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Cannot get a device boot status");
			mfst_var_boot_status = BOOT_STATUS_CANNOT_BOOT;
		}
	}

	return mfst_var_boot_status;
}

void device_healthcheck(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	char *img_set = NULL;
	uint32_t mfst_var_boot_status = BOOT_STATUS_CANNOT_BOOT;
	uint32_t mfst_var_confirm_status = CONFIRMED;
	uint32_t id_var_confirm_set = CONFIG_ID_VAR_CONFIRM_SET_A;

	mfst_var_boot_status = device_boot_status_get();

	if (mfst_var_boot_status == BOOT_STATUS_CANNOT_BOOT) {
		return;
	}

	if (mfst_var_boot_status == BOOT_STATUS_A_NO_RADIO
	    || mfst_var_boot_status == BOOT_STATUS_B_NO_RADIO) {
		/* Disable flash synchronization with radio core,
		 * as radio core is not running.
		 */
		flash_sync_rpc_host_sync_enable(false);
	}

	/* Confirming only in non-degraded boot states
	 */
	if (DT_SAME_NODE(DT_ALIAS(suit_active_code_partition),
			 DT_NODELABEL(cpuapp_slot_a_partition))) {
		if (mfst_var_boot_status != BOOT_STATUS_A) {
			return;
		}
		img_set = "A";
	} else {
		if (mfst_var_boot_status != BOOT_STATUS_B) {
			return;
		}

		img_set = "B";
		id_var_confirm_set = CONFIG_ID_VAR_CONFIRM_SET_B;
	}

	err = suit_mfst_var_get(id_var_confirm_set, &mfst_var_confirm_status);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Cannot get a confirm status");
		return;
	}

	if (mfst_var_confirm_status != CONFIRMED) {
		LOG_INF("Image set %s not confirmed yet, testing...", img_set);

		bool healthy = true;

		if (!radio_domain_healthy()) {
			LOG_ERR("Radio domain is NOT healthy");
			/* Disable flash synchronization with radio core,
			 * as radio core is not operational.
			 */
			flash_sync_rpc_host_sync_enable(false);
			healthy = false;
		}

		if (!app_domain_healthy()) {
			LOG_ERR("App domain is NOT healthy");
			healthy = false;
		}

		if (!healthy) {
			LOG_ERR("Reboot the device to try to boot from previous firmware");
			return;
		}

		LOG_INF("Confirming...");
		err = suit_mfst_var_set(id_var_confirm_set, CONFIRMED);
		if (err == SUIT_PLAT_SUCCESS) {
			LOG_INF("Confirmed\n");
		}
	}
}

void toggle_preferred_boot_set(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	uint32_t mfst_var_boot_preference = 0;

	err = suit_mfst_var_get(CONFIG_ID_VAR_BOOT_PREFERENCE, &mfst_var_boot_preference);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Cannot get current boot preference");
		return;
	}

	if (mfst_var_boot_preference == BOOT_PREFERENCE_B) {
		LOG_INF("Changing a boot preference (b -> A)");
		mfst_var_boot_preference = BOOT_PREFERENCE_A;
	} else {
		LOG_INF("Changing a boot preference (a -> B)");
		mfst_var_boot_preference = BOOT_PREFERENCE_B;
	}

	err = suit_mfst_var_set(CONFIG_ID_VAR_BOOT_PREFERENCE, mfst_var_boot_preference);
	if (err == SUIT_PLAT_SUCCESS) {
		LOG_INF("restart the device to enforce\n");
	} else {
		LOG_ERR("Cannot set a boot preference");
	}
}

void device_boot_state_report(void)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	uint32_t mfst_var_boot_preference = 0;
	uint32_t mfst_var_boot_status = 0;
	uint32_t mfst_var_confirm_set_a = 0;
	uint32_t mfst_var_confirm_set_b = 0;

	if (device_is_in_recovery_mode()) {
		printk("Boot in RECOVERY mode, probably the root manifest is damaged!\n");
	}

	err = suit_mfst_var_get(CONFIG_ID_VAR_BOOT_PREFERENCE, &mfst_var_boot_preference);
	if (err == SUIT_PLAT_SUCCESS) {
		if (mfst_var_boot_preference == BOOT_PREFERENCE_B) {
			printk("Boot preference: set B\n");
		} else {
			printk("Boot preference: set A\n");
		}
	}

	mfst_var_boot_status = device_boot_status_get();

	if (mfst_var_boot_status == BOOT_STATUS_A) {
		printk("Boot status: image set A active\n");
	} else if (mfst_var_boot_status == BOOT_STATUS_B) {
		printk("Boot status: image set B active\n");
	} else if (mfst_var_boot_status == BOOT_STATUS_A_DEGRADED) {
		printk("Boot status: image set A active, degraded mode\n");
	} else if (mfst_var_boot_status == BOOT_STATUS_B_DEGRADED) {
		printk("Boot status: app image B active, degraded mode\n");
	} else if (mfst_var_boot_status == BOOT_STATUS_A_NO_RADIO) {
		printk("Boot status: app image A active, no radio, degraded mode\n");
	} else if (mfst_var_boot_status == BOOT_STATUS_B_NO_RADIO) {
		printk("Boot status: app image B active, no radio, degraded mode\n");
	} else {
		printk("Boot status: unrecognized\n");
	}

	err = suit_mfst_var_get(CONFIG_ID_VAR_CONFIRM_SET_A, &mfst_var_confirm_set_a);
	if (err == SUIT_PLAT_SUCCESS) {
		if (mfst_var_confirm_set_a == NOT_CONFIRMED) {
			printk("Confirm status set A: not confirmed\n");
		} else if (mfst_var_confirm_set_a == CONFIRMED) {
			printk("Confirm status set A: confirmed\n");
		} else {
			printk("Confirm status set A: just installed\n");
		}
	}

	err = suit_mfst_var_get(CONFIG_ID_VAR_CONFIRM_SET_B, &mfst_var_confirm_set_b);
	if (err == SUIT_PLAT_SUCCESS) {
		if (mfst_var_confirm_set_b == NOT_CONFIRMED) {
			printk("Confirm status set B: not confirmed\n");
		} else if (mfst_var_confirm_set_b == CONFIRMED) {
			printk("Confirm status set B: confirmed\n");
		} else {
			printk("Confirm status set B: just installed\n");
		}
	}
}
