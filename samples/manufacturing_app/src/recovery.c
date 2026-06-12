/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "recovery.h"
#include "mfg_log.h"

#include <zephyr/kernel.h>
#include <nrfx.h>

/*
 * Read UICR.ERASEPROTECT directly: available on privileged cpuapp.
 *
 * NRF_UICR_S->ERASEPROTECT[0].PROTECT0  @ 0xFFD060  (reset: 0xFFFFFFFF)
 * NRF_UICR_S->ERASEPROTECT[0].PROTECT1  @ 0xFFD07C  (reset: 0xFFFFFFFF)
 * Protected value: 0x50FA50FA
 */
static void log_erase_all_status(void)
{
	uint32_t p0 = NRF_UICR_S->ERASEPROTECT[0].PROTECT0;
	uint32_t p1 = NRF_UICR_S->ERASEPROTECT[0].PROTECT1;
	bool active = (p0 == 0x50FA50FAU) && (p1 == 0x50FA50FAU);

#if defined(CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE)
	MFG_LOG_KV("erase_all", active ? "blocked" : "available");
#else
	MFG_LOG_INF("Erase-all: %s\n",
		    active ? "unavailable (blocked by UICR.ERASEPROTECT)"
			   : "available (UICR.ERASEPROTECT not active)");
#endif
}

static void log_auth_erase_all_status(bool keys_provisioned)
{
#if defined(CONFIG_SAMPLE_MFG_LOG_MACHINE_READABLE)
	MFG_LOG_KV("auth_erase_all", keys_provisioned ? "available" : "unavailable");
#else
	if (keys_provisioned) {
		MFG_LOG_INF("Authenticated erase-all: available\n");
	} else {
		MFG_LOG_INF("Authenticated erase-all: unavailable (keys are not provisioned yet)\n");
	}
#endif
}

__attribute__((noreturn))
void recovery_suspend(bool keys_provisioned)
{
	MFG_LOG_INF("\nTry to recover the device.\n");
	log_erase_all_status();
	log_auth_erase_all_status(keys_provisioned);
	MFG_LOG_INF("Execution suspended.\n");
	MFG_LOG_DONE(false);

	/* Infinite loop, the device must be reset externally. */
	while (1) {
		k_sleep(K_MSEC(1000));
	}

	/* Unreachable, but required for __attribute__((noreturn)). */
	CODE_UNREACHABLE;
}
