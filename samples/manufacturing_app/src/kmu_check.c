/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Step 2 — Validate that no IKG seed or KeyRAM random data is already present.
 *
 * Check strategy:
 *
 * IKG seed presence:
 *   hw_unique_key_are_any_written() directly queries the KMU slot for the
 *   CRACEN_KMU_KEY_USAGE_SCHEME_SEED entry via cracen_kmu_get_key_slot().
 *   This is more reliable than trying to derive the IAK via psa_export_public_key(),
 *   which returns driver-specific error codes that vary depending on heap state
 *   and CRACEN configuration.
 *     - returns true  → seed IS present (error — must suspend).
 *     - returns false → seed not present (good).
 *
 * KeyRAM random slots (PROTECTED_RAM_INVALIDATION_DATA_SLOT1 = 248,
 *                      PROTECTED_RAM_INVALIDATION_DATA_SLOT2 = 249):
 *   We probe the KMU slot directly via cracen_kmu_get_key_slot():
 *     - PSA_SUCCESS                → slot occupied (error — must suspend).
 *     - any other status           → slot empty (good).
 */

#include "kmu_check.h"
#include "mfg_log.h"
#include "recovery.h"

#include <psa/crypto.h>
#include <cracen/cracen_kmu.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>
#include <hw_unique_key.h>

/* ---------------------------------------------------------------------------
 * IKG seed presence check via direct KMU slot query
 * ---------------------------------------------------------------------------
 */
static void check_ikg_seed_slots(void)
{
	MFG_LOG_INF("Verifying KMU IKG seed slots...");

	if (hw_unique_key_are_any_written()) {
		MFG_LOG_INF(" FAIL (not empty)\n");
		MFG_LOG_ERR("\nSeems IKG seed is already provisioned. Secrecy and entropy level\n");
		MFG_LOG_ERR("of that data cannot be trusted.\n");
		recovery_suspend(false);
	}

	MFG_LOG_INF(" OK (empty)\n");
}

/* ---------------------------------------------------------------------------
 * KeyRAM random slot presence check
 *
 * cracen_kmu_get_key_slot() is the same direct KMU probe used internally by
 * hw_unique_key_are_any_written(). It returns PSA_SUCCESS if and only if the
 * slot holds a valid key, without performing any cryptographic operation.
 *
 * Slot numbers are defined in cracen_kmu.h:
 *   PROTECTED_RAM_INVALIDATION_DATA_SLOT1 = 248
 *   PROTECTED_RAM_INVALIDATION_DATA_SLOT2 = 249
 * ---------------------------------------------------------------------------
 */
static void check_one_keyram_slot(int slot_id)
{
	MFG_LOG_INF("Verifying KMU KeyRAM random slot %d...", slot_id);

	psa_key_id_t key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, slot_id);

	psa_key_lifetime_t lifetime;
	psa_drv_slot_number_t slot_number;
	psa_status_t status = cracen_kmu_get_key_slot(key_id, &lifetime, &slot_number);

	if (status != PSA_SUCCESS) {
		MFG_LOG_INF(" OK (empty)\n");
		return;
	}

	MFG_LOG_INF(" FAIL (not empty)\n");
	MFG_LOG_ERR("\nSeems KeyRAM random (slot %d) is already provisioned.\n", slot_id);
	MFG_LOG_ERR("Secrecy and entropy level of that data cannot be trusted.\n");
	recovery_suspend(false);
}

/* ---------------------------------------------------------------------------
 * Step 2 entry point
 * ---------------------------------------------------------------------------
 */
void step2_kmu_check_empty(void)
{
	MFG_LOG_STEP("Checking KMU slots");

	check_ikg_seed_slots();

	check_one_keyram_slot(PROTECTED_RAM_INVALIDATION_DATA_SLOT1);
	check_one_keyram_slot(PROTECTED_RAM_INVALIDATION_DATA_SLOT2);
}
