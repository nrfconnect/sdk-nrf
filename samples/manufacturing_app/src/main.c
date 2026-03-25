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
 *   NRF_LCS_ASSEMBLY_AND_TEST     -> assembly_and_test_handler()  (Steps 2–6)
 *   NRF_LCS_PSA_ROT_PROVISIONING  -> psa_rot_provisioning_handler() (Steps 7–12)
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
#include <nrf_lcs/nrf_lcs.h>

#include "mfg_key_blob.h"
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

static const char *lcs_to_str(enum nrf_lcs state)
{
	switch (state) {
	case NRF_LCS_ASSEMBLY_AND_TEST:    return "Assembly and Test";
	case NRF_LCS_PSA_ROT_PROVISIONING: return "PSA RoT Provisioning";
	case NRF_LCS_SECURED:              return "Secured";
	case NRF_LCS_DECOMMISSIONED:       return "Decommissioned";
	case NRF_LCS_UNKNOWN:              return "Unknown";
	default:                           return "<invalid>";
	}
}

static void step1_lcs_check(void)
{
	enum nrf_lcs lcs = nrf_lcs_get();

	MFG_LOG_STEP("Checking LCS");
	MFG_LOG_KV("lcs", lcs_to_str(lcs));

	if (lcs != NRF_LCS_ASSEMBLY_AND_TEST &&
	    lcs != NRF_LCS_PSA_ROT_PROVISIONING) {
		MFG_LOG_ERR("Manufacturing application is not allowed to run in LCS '%s'.\n",
			    lcs_to_str(lcs));
		MFG_LOG_ERR("Expected ASSEMBLY_AND_TEST or PSA_ROT_PROVISIONING. "
			    "If LCS is SECURED or DECOMMISSIONED this indicates "
			    "a serious defect in the second-stage bootloader.\n");
		recovery_suspend(false);
	}
}

/* Wrap nrf_lcs_set() with a read-back so that a silently failed
 * transition (glitch / tamper / hardware fault) suspends immediately
 * instead of letting the caller continue in the wrong state.
 */
static void lcs_set_and_verify(enum nrf_lcs target, const char *step_name)
{
	enum nrf_lcs actual;

	MFG_LOG_STEP(step_name);
	nrf_lcs_set(target);

	actual = nrf_lcs_get();
	MFG_LOG_KV("lcs", lcs_to_str(actual));

	if (actual != target) {
		MFG_LOG_ERR("LCS transition failed: requested '%s', read back '%s'\n",
			    lcs_to_str(target), lcs_to_str(actual));
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
	lcs_set_and_verify(NRF_LCS_PSA_ROT_PROVISIONING,
			   "Transitioning LCS to 'PSA RoT Provisioning'");

	return 0;
}

static int psa_rot_provisioning_handler(void)
{
	/* Step 7: Provision KeyRAM random data */
	step7_provision_keyram_random();

	/* Step 8: Provision IKG seed */
	step8_huk_provision_ikg_seed();

	/* Step 9: End product tests */
	step9_tests_run();

	/* Step 10: Provision remaining assets */
	step10_assets_provision();

	/* Step 11: Advance LCS to 'Secured' */
	lcs_set_and_verify(NRF_LCS_SECURED,
			   "Transitioning LCS to 'Secured'");

	/* Step 12: Revoke manufacturing app key and reboot. */
	MFG_LOG_STEP("Cleaning up");
	step12_key_revoke_mfg_key();

	MFG_LOG_INF("\nManufacturing process is completed. The device is about to reboot\n");
	MFG_LOG_INF("and the target application will be installed.\n");
	MFG_LOG_DONE(true);

	k_sleep(K_MSEC(100));
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

int main(void)
{
	if (mfg_key_blob_get() == NULL) {
		MFG_LOG_ERR("mfg_keys partition is missing or invalid.\n");
		MFG_LOG_ERR("Flash mfg_keys.bin to the mfg_keys partition (see README).\n");
		recovery_suspend(false);
	}

	/*
	 * TODO: Remove once BL1/NSIB provisions the manufacturing app key.
	 * CONFIG_SAMPLE_MFG_APP_KEY_KMU_SLOT before this application runs. This call
	 * is a temporary workaround that makes the sample self-contained.
	 */
	key_provision_mfg_app_key();

	/* Step 1: Check LCS. nrf_lcs_get() reads OTP directly; no PSA init required. */
	step1_lcs_check();

	if (nrf_lcs_get() == NRF_LCS_ASSEMBLY_AND_TEST) {
		int ret = assembly_and_test_handler();

		if (ret != 0) {
			MFG_LOG_ERR("Assembly and test handler failed (err=%d)\n", ret);
			MFG_LOG_DONE(false);
			return ret;
		}
	}

	if (nrf_lcs_get() == NRF_LCS_PSA_ROT_PROVISIONING) {
		int ret = psa_rot_provisioning_handler();

		if (ret != 0) {
			MFG_LOG_ERR("PSA RoT provisioning handler failed (err=%d)\n", ret);
			MFG_LOG_DONE(false);
			return ret;
		}

		return 0;
	}

	MFG_LOG_ERR("Device in an unbootable LCS: %s\n", lcs_to_str(nrf_lcs_get()));
	recovery_suspend(false);
}
