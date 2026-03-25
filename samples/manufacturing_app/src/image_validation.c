/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Step 3: Image digest validation.
 *
 * TODO
 *
 */

#include "image_validation.h"
#include "mfg_log.h"
#include "recovery.h"

#include <psa/crypto.h>
#include <image_digests.h>

/* ---------------------------------------------------------------------------
 * Digest comparison helper
 * ---------------------------------------------------------------------------
 */
static bool __attribute__((unused)) digests_equal(const uint8_t *first_digest,
						  const uint8_t *second_digest,
						  size_t len)
{
	uint8_t diff = 0;

	for (size_t i = 0; i < len; i++) {
		diff |= first_digest[i] ^ second_digest[i];
	}
	return diff == 0;
}

/* ---------------------------------------------------------------------------
 * BL1 validation
 * ---------------------------------------------------------------------------
 */
static void validate_bl1(void)
{
	MFG_LOG_INF("Validating Bootloader 1...");

	if (mfg_digest_is_zero(mfg_bl1_expected_digest)) {
		MFG_LOG_INF(" SKIP (no expected digest provided at build time)\n");
		MFG_LOG_INF("  Provide -DMFG_BL1_DIGEST=<sha256hex> to enable BL1 validation.\n");
		return;
	}
	/* TODO*/

	MFG_LOG_INF(" SKIP (partition address integration not yet implemented)\n");
}

/* ---------------------------------------------------------------------------
 * BL2 (MCUboot) slot A validation
 * ---------------------------------------------------------------------------
 */
static void validate_bl2_slot_a(void)
{
	MFG_LOG_INF("Validating Bootloader 2, slot A...");

	if (mfg_digest_is_zero(mfg_bl2_slot_a_expected_digest)) {
		MFG_LOG_INF(" SKIP (no expected digest provided at build time)\n");
		return;
	}

	/* TODO */
	MFG_LOG_INF(" SKIP (partition address integration not yet implemented)\n");
}

/* ---------------------------------------------------------------------------
 * BL2 (MCUboot) slot B validation
 * ---------------------------------------------------------------------------
 */
static void validate_bl2_slot_b(void)
{
	MFG_LOG_INF("Validating Bootloader 2, slot B...");

	if (mfg_digest_is_zero(mfg_bl2_slot_b_expected_digest)) {
		MFG_LOG_INF(" SKIP (no expected digest provided at build time)\n");
		return;
	}

	/* TODO */
	MFG_LOG_INF(" SKIP (partition address integration not yet implemented)\n");
}

/* ---------------------------------------------------------------------------
 * Manufacturing application self-validation
 * ---------------------------------------------------------------------------
 */
static void validate_manufacturing_app(void)
{
	MFG_LOG_INF("Validating Manufacturing application (self-check)...");

	/* TODO*/
	MFG_LOG_INF(" SKIP (manufacturing applicatio validation not yet implemented)\n");
}

/* ---------------------------------------------------------------------------
 * Application candidate validation
 * ---------------------------------------------------------------------------
 */
static void validate_app_candidate(void)
{
	MFG_LOG_INF("Validating Application candidate...");

	if (mfg_digest_is_zero(mfg_app_candidate_expected_digest)) {
		MFG_LOG_INF(" SKIP (no expected digest provided at build time)\n");
		MFG_LOG_INF("  Provide -DMFG_APP_CANDIDATE_DIGEST=<sha256hex> to enable.\n");
		return;
	}

	/* TODO*/

	MFG_LOG_INF(" SKIP (flash-map read not yet implemented)\n");
}

/* ---------------------------------------------------------------------------
 * Step 3 entry point
 * ---------------------------------------------------------------------------
 */
void step3_image_validate_all(void)
{
	MFG_LOG_STEP("Validating images");

	validate_bl1();
	validate_bl2_slot_a();
	validate_bl2_slot_b();
	validate_manufacturing_app();
	validate_app_candidate();
}
