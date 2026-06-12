/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * Step 5: Block erase-all via UICR.ERASEPROTECT.
 *
 * Hardware background (nRF54LV10A):
 *   UICR.ERASEPROTECT register pair (UICR base 0xFFD000):
 *     NRF_UICR_S->ERASEPROTECT[0].PROTECT0  @ 0xFFD060  (reset: 0xFFFFFFFF)
 *     NRF_UICR_S->ERASEPROTECT[0].PROTECT1  @ 0xFFD07C  (reset: 0xFFFFFFFF)
 *
 *   Valid states:
 *     0xFFFFFFFF / 0xFFFFFFFF — protection NOT active (erase-all allowed)
 *     0x50FA50FA / 0x50FA50FA — protection active (erase-all blocked)
 *
 *   Any other value is suspicious and logged as an error.
 *
 * Write mechanism:
 *   Writes use nrfx_rramc_word_write() with the direct register address.
 *   The write is one-time (OTP-like) because once set to 0x50FA50FA the ERASEPROTECT
 *   value can only be cleared by an authenticated erase-all.
 */

#include "erase_protection.h"
#include "mfg_log.h"
#include "recovery.h"

#include <autoconf.h>
#include <nrfx_rramc.h>
#include <nrfx.h>

#define ERASEPROTECT_ACTIVE_VALUE   0x50FA50FAU
#define ERASEPROTECT_INACTIVE_VALUE 0xFFFFFFFFU

/* ---------------------------------------------------------------------------
 * Read UICR.ERASEPROTECT registers directly (privileged MMIO).
 * ---------------------------------------------------------------------------
 */
static void eraseprotect_read(uint32_t *protect0, uint32_t *protect1)
{
	*protect0 = NRF_UICR_S->ERASEPROTECT[0].PROTECT0;
	*protect1 = NRF_UICR_S->ERASEPROTECT[0].PROTECT1;
}

/* ---------------------------------------------------------------------------
 * Write UICR.ERASEPROTECT registers via RRAMC word write.
 * Only used when CONFIG_SAMPLE_BLOCK_ERASE_ALL is enabled.
 * ---------------------------------------------------------------------------
 */
#ifdef CONFIG_SAMPLE_BLOCK_ERASE_ALL
static int eraseprotect_activate(void)
{
	nrfx_rramc_word_write((uint32_t)&NRF_UICR_S->ERASEPROTECT[0].PROTECT0,
			      ERASEPROTECT_ACTIVE_VALUE);
	nrfx_rramc_word_write((uint32_t)&NRF_UICR_S->ERASEPROTECT[0].PROTECT1,
			      ERASEPROTECT_ACTIVE_VALUE);
	return 0;
}
#endif /* CONFIG_SAMPLE_BLOCK_ERASE_ALL */

/* ---------------------------------------------------------------------------
 * Step 5 entry point
 * ---------------------------------------------------------------------------
 */
void step5_erase_block_if_needed(void)
{
	MFG_LOG_STEP("Checking erase-all policy");

#ifdef CONFIG_SAMPLE_BLOCK_ERASE_ALL
	MFG_LOG_INF("CONFIG_SAMPLE_BLOCK_ERASE_ALL is active\n");
#else
	MFG_LOG_INF("CONFIG_SAMPLE_BLOCK_ERASE_ALL is not active\n");
#endif

	uint32_t protect0 = 0;
	uint32_t protect1 = 0;

	eraseprotect_read(&protect0, &protect1);

	MFG_LOG_INF("UICR.ERASEPROTECT.PROTECT0 = 0x%08X\n", protect0);
	MFG_LOG_INF("UICR.ERASEPROTECT.PROTECT1 = 0x%08X\n", protect1);

	bool p0_active   = (protect0 == ERASEPROTECT_ACTIVE_VALUE);
	bool p1_active   = (protect1 == ERASEPROTECT_ACTIVE_VALUE);
	bool p0_inactive = (protect0 == ERASEPROTECT_INACTIVE_VALUE);
	bool p1_inactive = (protect1 == ERASEPROTECT_INACTIVE_VALUE);
	bool p0_valid    = p0_active || p0_inactive;
	bool p1_valid    = p1_active || p1_inactive;

	/* ----- Suspicious values ----- */
	if (!p0_valid || !p1_valid) {
		MFG_LOG_ERR("\nSuspicious UICR.ERASEPROTECT value detected.\n");
		MFG_LOG_ERR("Expected 0xFFFFFFFF (inactive) or 0x50FA50FA (active).\n");
		recovery_suspend(false);
	}

	bool currently_active = p0_active && p1_active;

#ifdef CONFIG_SAMPLE_BLOCK_ERASE_ALL

	if (currently_active) {
		MFG_LOG_INF("erase-all is blocked in UICR, nothing to do.\n");
		return;
	}

	MFG_LOG_INF("\nBlocking erase-all...\n");

	int err = eraseprotect_activate();

	if (err != 0) {
		MFG_LOG_ERR("Failed to activate ERASEPROTECT (err=%d).\n", err);
		recovery_suspend(false);
	}

	MFG_LOG_INF("Verifying erase-all...\n");

	eraseprotect_read(&protect0, &protect1);

	MFG_LOG_INF("UICR.ERASEPROTECT.PROTECT0 = 0x%08X\n", protect0);
	MFG_LOG_INF("UICR.ERASEPROTECT.PROTECT1 = 0x%08X\n", protect1);

	if (protect0 != ERASEPROTECT_ACTIVE_VALUE || protect1 != ERASEPROTECT_ACTIVE_VALUE) {
		MFG_LOG_ERR("\nFailed to activate UICR.ERASEPROTECT.\n");
		recovery_suspend(false);
	}

	MFG_LOG_INF("\nerase-all is successfully blocked in UICR.\n");

#else /* CONFIG_SAMPLE_BLOCK_ERASE_ALL not set */

	if (!currently_active) {
		MFG_LOG_INF("erase-all is not blocked in UICR, nothing to do.\n");
		MFG_LOG_INF("\nNotice: the device is not protected against erase-all.\n");
		MFG_LOG_INF("Anyone with SWD access can wipe the RRAM content.\n");
		MFG_LOG_INF("Expected for engineering devices; consider enabling for end-devices.\n");
	} else {
		MFG_LOG_INF("erase-all is blocked in UICR.\n");
	}

#endif /* CONFIG_SAMPLE_BLOCK_ERASE_ALL */
}
