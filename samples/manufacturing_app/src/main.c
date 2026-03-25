/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file main.c
 *
 * Manufacturing application: 12-step provisioning flow for nRF54LV10A.
 *
 * Overview
 * --------
 * This application is a reference implementation of the manufacturing flow
 * for Nordic Semiconductor devices using CRACEN.
 *
 * LCS-aware execution
 * -------------------
 * The main switch dispatches to one of two handlers based on the current
 * Lifecycle State:
 *
 *   LCS_ASSEMBLY_AND_TEST     -> assembly_and_test_handler()  (Steps 2–6)
 *   LCS_PSA_ROT_PROVISIONING  -> psa_rot_provisioning_handler() (Steps 7–12)
 *
 * Execution flow
 * --------------
 * Assembly and Test state:
 *   Step  2: Validate no IKG seed or KeyRAM random present.
 *   Step  3: Validate digests of all sub-images.
 *   Step  4: Provision and verify secure-boot and ADAC public keys.
 *   Step  5: Block erase-all via UICR.ERASEPROTECT (if CONFIG_SAMPLE_BLOCK_ERASE_ALL).
 *   Step  6: Advance LCS to 'PROT Provisioning'.
 *
 * PROT Provisioning state:
 *   Step  7: Provision KeyRAM random data (KMU slots 248, 249).
 *   Step  8: Provision IKG seed; log IAK / MKEK / MEXT identifiers.
 *   Step  9: End product tests (placeholder: customise for your product).
 *   Step 10: Provision remaining assets and device enrolment (placeholder).
 *   Step 11: Advance LCS to 'Secured'.
 *   Step 12: Revoke manufacturing app key; reboot to install target application.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <lcs.h>

#include "mfg_log.h"
#include "recovery.h"
#include "kmu_check.h"
#include "image_validation.h"
#include "key_provisioning.h"
#include "erase_protection.h"
#include "huk_provisioning.h"
#include "product_tests.h"
#include "asset_provisioning.h"

LOG_MODULE_REGISTER(manufacturing_app, LOG_LEVEL_DBG);

static const char *lcs_to_str(enum lcs state)
{
	switch (state) {
	case LCS_ASSEMBLY_AND_TEST:    return "Assembly and Test";
	case LCS_PSA_ROT_PROVISIONING: return "PSA RoT Provisioning";
	case LCS_SECURED:              return "Secured";
	case LCS_DECOMMISSIONED:       return "Decommissioned";
	case LCS_UNKNOWN:              return "Unknown";
	default:                       return "<invalid>";
	}
}

static void step1_lcs_check(void)
{
	MFG_LOG_STEP("Checking LCS");
	MFG_LOG_KV("lcs", lcs_to_str(lcs_get()));

	if (lcs_get() == LCS_SECURED) {
		MFG_LOG_ERR("Manufacturing application shall not be executed in that state.\n");
		MFG_LOG_ERR("Situation like this indicates a serious defect in 2nd stage bootloader.\n");
		recovery_suspend(false);
	}
}

static int assembly_and_test_handler(void)
{
	/* Step 2: Validate no IKG seed or KeyRAM random present */
	step2_kmu_check_empty();

	/* Step 3: Validate sub-image digests */
	step3_image_validate_all();

	/* Step 4: Provision secure-boot and ADAC keys */
	step4_key_provision_all();

	/* Step 5: Block erase-all via UICR.ERASEPROTECT */
	step5_erase_block_if_needed();

	/* Step 6: Advance LCS to 'PROT Provisioning' */
	MFG_LOG_STEP("Transitioning LCS to 'PSA RoT Provisioning'");
	lcs_set(LCS_PSA_ROT_PROVISIONING);
	MFG_LOG_KV("lcs", lcs_to_str(lcs_get()));

	return 0;
}

static int psa_rot_provisioning_handler(void)
{
	/* Step 7: Provision KeyRAM random data */
	step7_huk_provision_keyram_random();

	/* Step 8: Provision IKG seed */
	step8_huk_provision_ikg_seed();

	/* Step 9: End product tests */
	step9_tests_run();

	/* Step 10: Provision remaining assets */
	step10_assets_provision();

	/* Step 11: Advance LCS to 'Secured' */
	MFG_LOG_STEP("Transitioning LCS to 'Secured'");
	lcs_set(LCS_SECURED);
	MFG_LOG_KV("lcs", lcs_to_str(lcs_get()));

	/* Step 12: Revoke manufacturing app key and reboot */
	MFG_LOG_STEP("Cleaning up");
	step12_key_revoke_mfg_key();

	MFG_LOG_INF("\nManufacturing process is completed. The device is about to reboot\n");
	MFG_LOG_INF("and the target application will be installed.\n");
	MFG_LOG_DONE(true);

	/* Allow UART to flush before reboot. */
	k_sleep(K_MSEC(100));
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

int main(void)
{
	/*
	 * TODO: Remove once BL1/NSIB provisions the manufacturing app key.
	 * CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT before this application runs. This call
	 * is a temporary workaround that makes the sample self-contained.
	 */
	key_provision_mfg_app_key();

	/* Step 1: Check LCS. lcs_get() reads OTP directly; no PSA init required. */
	step1_lcs_check();

	if (lcs_get() == LCS_ASSEMBLY_AND_TEST) {
		int ret = assembly_and_test_handler();

		if (ret != 0) {
			MFG_LOG_ERR("Assembly and test handler failed (err=%d)\n", ret);
			MFG_LOG_DONE(false);
			return ret;
		}
	}

	if (lcs_get() == LCS_PSA_ROT_PROVISIONING) {
		int ret = psa_rot_provisioning_handler();

		if (ret != 0) {
			MFG_LOG_ERR("PSA RoT provisioning handler failed (err=%d)\n", ret);
			MFG_LOG_DONE(false);
			return ret;
		}

		return 0;
	}

	MFG_LOG_ERR("Device in an unbootable LCS: %s\n", lcs_to_str(lcs_get()));
	recovery_suspend(false);
}
