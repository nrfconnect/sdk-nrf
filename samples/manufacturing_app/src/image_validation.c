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
#include "mfg_key_blob.h"
#include "mfg_log.h"
#include "recovery.h"

#include <psa/crypto.h>

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

static void validate_image_digest(const char *label, const uint8_t *digest)
{
	MFG_LOG_INF("Validating %s...", label);

	if (digest == NULL || mfg_digest_is_zero(digest)) {
		MFG_LOG_INF(" SKIP (no expected digest provided at build time)\n");
		return;
	}

	/* TODO: read the image from its partition, hash it, compare to digest. */
	MFG_LOG_INF(" SKIP (partition address integration not yet implemented)\n");
}

static void validate_manufacturing_app(void)
{
	MFG_LOG_INF("Validating Manufacturing application (self-check)...");
	/* TODO */
	MFG_LOG_INF(" SKIP (self-check not yet implemented)\n");
}

/* ---------------------------------------------------------------------------
 * Step 3 entry point
 * ---------------------------------------------------------------------------
 */
void step3_image_validate_all(void)
{
	MFG_LOG_STEP("Validating images");

	const struct mfg_key_blob *blob = mfg_key_blob_get();

	validate_image_digest("Bootloader 1",
			      blob ? blob->bl1_digest : NULL);
	validate_image_digest("Bootloader 2, slot A",
			      blob ? blob->bl2_slot_a_digest : NULL);
	validate_image_digest("Bootloader 2, slot B",
			      blob ? blob->bl2_slot_b_digest : NULL);
	validate_manufacturing_app();
	validate_image_digest("Application candidate",
			      blob ? blob->app_candidate_digest : NULL);
}
