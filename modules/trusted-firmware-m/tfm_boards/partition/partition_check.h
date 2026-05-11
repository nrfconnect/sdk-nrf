/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TFM_TZ_PARTITION_CHECK_H__
#define __TFM_TZ_PARTITION_CHECK_H__

/**
 * Runtime sanity check for the TrustZone partition layout.
 *
 * Verifies that the {code,data}-partitions referenced via every okay
 * "nordic,tz-{secure,nonsecure}" descriptor form a single contiguous
 * block per security state, that secure starts at SECURE_IMAGE_OFFSET,
 * and that non-secure begins exactly where secure ends.
 *
 * Call once from platform init. Compile-time invariants on the same
 * layout (equal slot sizes, total flash fit) are enforced via
 * BUILD_ASSERT in partition_check.c.
 */
void tfm_tz_partition_check(void);

#endif /* __TFM_TZ_PARTITION_CHECK_H__ */
