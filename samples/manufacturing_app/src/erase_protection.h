/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file erase_protection.h
 *
 * Step 5: Check and optionally block the SWD erase-all command via
 * UICR.ERASEPROTECT on the nRF54LV10A.
 */

#ifndef ERASE_PROTECTION_H_
#define ERASE_PROTECTION_H_

/**
 * @brief Step 5: Check and optionally block erase-all.
 *
 * Reads UICR.ERASEPROTECT.PROTECT0 and PROTECT1 and logs their content.
 * If CONFIG_SAMPLE_BLOCK_ERASE_ALL is enabled and the protection is not yet active,
 * programs the protection values via nrfx_rramc_word_write() and verifies.
 *
 */
void step5_erase_block_if_needed(void);

#endif /* ERASE_PROTECTION_H_ */
