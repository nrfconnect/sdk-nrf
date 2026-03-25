/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Steps 7 and 8: Provision KeyRAM random data and IKG seed.
 *
 * Step 7: KeyRAM random (PROTECTED scheme):
 *   KMU slots 248 (PROTECTED_RAM_INVALIDATION_DATA_SLOT1) and
 *             249 (PROTECTED_RAM_INVALIDATION_DATA_SLOT2).
 *
 *   These are AES keys that CRACEN uses to overwrite its protected RAM after
 *   each key use. They are provisioned by calling cracen_provision_prot_ram_inv_slots()
 *   directly, which generates 256 bits of hardware random data and populates
 *   both slots atomically. CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT=y
 *   ensures this function is compiled into the driver.
 *
 * Step 8: IKG seed (SEED scheme):
 *   Stored in CONFIG_CRACEN_IKG_SEED_KMU_SLOT using CRACEN_KMU_KEY_USAGE_SCHEME_SEED.
 *   From this seed CRACEN derives:
 *     IAK  (Initial Attestation Key)   — 0x7FFFC001
 *     MKEK (Master Key Encryption Key) — 0x7FFFC002
 *     MEXT (Master External Key)       — 0x7FFFC003
 *
 */

#include "huk_provisioning.h"
#include "mfg_log.h"
#include "recovery.h"

#include <zephyr/kernel.h>
#include <psa/crypto.h>
#include <cracen/cracen_kmu.h>
#include <cracen_psa_key_ids.h>
#include <hw_unique_key.h>
#include <provisioned_keys.h>

/* ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------
 */

static bool ikg_seed_is_provisioned(void)
{
	return hw_unique_key_are_any_written();
}

static bool all_zeros(const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (data[i] != 0U) {
			return false;
		}
	}
	return true;
}

/* ---------------------------------------------------------------------------
 * Step 7: Provision KeyRAM random data
 *
 * cracen_provision_prot_ram_inv_slots() generates 256 bits of hardware random
 * data and provisions both slots 248 and 249 atomically.
 *
 * CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT=y ensures this
 * function is compiled into the CRACEN driver.
 *
 * TODO: Update to use externally provided random data once the CRACEN driver
 * supports importing caller-supplied material into PROTECTED-scheme slots.
 * ---------------------------------------------------------------------------
 */
void step7_huk_provision_keyram_random(void)
{
	MFG_LOG_STEP("Provisioning KeyRAM random data");
	MFG_LOG_INF("Provisioning KeyRAM random (slots %d and %d)...",
		    PROTECTED_RAM_INVALIDATION_DATA_SLOT1,
		    PROTECTED_RAM_INVALIDATION_DATA_SLOT2);

	psa_status_t status = cracen_provision_prot_ram_inv_slots();

	if (status != PSA_SUCCESS) {
		MFG_LOG_INF(" FAIL (%d)\n", status);
		MFG_LOG_ERR("Failed to provision KeyRAM random slots.\n");
		recovery_suspend(false);
	}

	MFG_LOG_INF(" OK (hardware-generated random)\n");
}

/* ---------------------------------------------------------------------------
 * Verify the IKG seed is functional by exporting the IAK public key.
 * If the seed was written with bad data (e.g. all-zeros), CRACEN cannot
 * derive the IAK and psa_export_public_key() will fail.
 * ---------------------------------------------------------------------------
 */
static void verify_ikg_seed(void)
{
	/* IAK is a secp256r1 (P-256) key pair. Uncompressed public point = 65 bytes. */
	uint8_t iak_pub[65];
	size_t  iak_pub_len = 0;

	psa_status_t status = psa_export_public_key(CRACEN_BUILTIN_IDENTITY_KEY_ID,
						    iak_pub, sizeof(iak_pub),
						    &iak_pub_len);

	if (status != PSA_SUCCESS) {
		MFG_LOG_ERR("IKG seed verification FAILED: psa_export_public_key(IAK) = %d\n",
			    status);
		MFG_LOG_ERR("The IKG seed may not have been provisioned correctly.\n");
		recovery_suspend(false);
	}

	MFG_LOG_INF("IKG-derived key identifiers:\n");
	MFG_LOG_INF("  IAK  (0x%08x, secp256r1, %zu bytes):\n",
		    (unsigned int)CRACEN_BUILTIN_IDENTITY_KEY_ID, iak_pub_len);
	mfg_log_bytes("iak_pubkey", iak_pub, iak_pub_len);
	MFG_LOG_INF("  MKEK (0x%08x)\n", (unsigned int)CRACEN_BUILTIN_MKEK_ID);
	MFG_LOG_INF("  MEXT (0x%08x)\n", (unsigned int)CRACEN_BUILTIN_MEXT_ID);
}

/* ---------------------------------------------------------------------------
 * Step 8: Provision IKG seed and verify derived key availability
 * ---------------------------------------------------------------------------
 */
void step8_huk_provision_ikg_seed(void)
{
	MFG_LOG_STEP("Provisioning IKG seed");

	if (ikg_seed_is_provisioned()) {
		MFG_LOG_INF("IKG seed already provisioned — verifying derivation.\n");
		verify_ikg_seed();
		return;
	}

	BUILD_ASSERT(sizeof(ikg_seed) == HUK_SIZE_BYTES,
		     "ikg_seed.bin must be exactly HUK_SIZE_BYTES (48 bytes)");

	if (all_zeros(ikg_seed, sizeof(ikg_seed))) {
		MFG_LOG_INF("\nWARNING: ikg_seed.bin is all-zeros.\n");
		MFG_LOG_INF("Replace the placeholder binary with per-device random data.\n");
		MFG_LOG_INF("Continuing in 5 seconds...\n");
		k_sleep(K_SECONDS(5));
	}

	MFG_LOG_INF("Writing IKG seed to KMU slot %d...", CONFIG_CRACEN_IKG_SEED_KMU_SLOT);

	/* hw_unique_key_write() wraps cracen_import_key with CRACEN_KMU_KEY_USAGE_SCHEME_SEED.
	 * The key_slot parameter is unused on CRACEN. The slot is always
	 * CONFIG_CRACEN_IKG_SEED_KMU_SLOT. We pass HUK_KEYSLOT_MKEK as a placeholder.
	 */
	int err = hw_unique_key_write(HUK_KEYSLOT_MKEK, ikg_seed);

	if (err != HW_UNIQUE_KEY_SUCCESS) {
		MFG_LOG_INF(" FAIL (hw_unique_key_write returned %d)\n", err);
		MFG_LOG_ERR("Failed to provision IKG seed.\n");
		recovery_suspend(false);
	}

	MFG_LOG_INF(" OK\n");
	verify_ikg_seed();
}
