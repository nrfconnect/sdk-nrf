/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file kmu_check.h
 *
 * Step 2 — Validate that no IKG seed or KeyRAM random data is already
 * present in KMU in LCS = 'Manufacturing and Test'.
 *
 * A pre-existing seed would indicate either a leftover from previous
 * activity on the device or an intentional malicious action. Either case
 * is a serious security flaw and manufacturing must halt.
 */

#ifndef KMU_CHECK_H_
#define KMU_CHECK_H_

/**
 * @brief Step 2 — Check that IKG seed and KeyRAM random slots are empty.
 *
 * Only call this function when LCS = 'Manufacturing and Test'. It verifies:
 *   1. The IKG seed KMU slots (used by CRACEN's internal key generator) are
 *      empty — confirmed via hw_unique_key_are_any_written().
 *   2. The KeyRAM random invalidation slots (248, 249) are empty.
 *
 * Suspends execution if either check finds pre-existing data.
 */
void step2_kmu_check_empty(void);

#endif /* KMU_CHECK_H_ */
