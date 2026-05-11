/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Validity checks for the TrustZone partition layout declared in devicetree.
 *
 * Lives in a .c file (not flash_layout.h) because the header is also pulled
 * in by linker scripts, which can't expand BUILD_ASSERT or the full
 * <zephyr/devicetree.h> macro set.
 *
 * Skipped under the Partition Manager where pm_static.yml is the source of
 * truth and FLASH_*_PARTITION_SIZE may legitimately diverge from DTS.
 *
 * Both nordic,tz-{secure,nonsecure} descriptors are guaranteed to exist in
 * any non-PM build that reaches this file: flash_layout.h #errors otherwise.
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#include "flash_layout.h"

#if !defined(CONFIG_PARTITION_MANAGER_ENABLED)

struct tfm_tz_part {
	uint32_t start;
	uint32_t end;
};

struct tfm_tz_summary {
	uint32_t min_start;
	uint32_t max_end;
	uint32_t sum_size;
};

#define TFM_TZ_PART_FROM_PH(node_id, prop, idx)                                                    \
	{                                                                                          \
		.start = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                       \
		.end = DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)) +                        \
		       DT_REG_SIZE(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                         \
	},

/* data-partitions is optional. */
#define TFM_TZ_PARTS_FROM_NODE(node_id)                                                            \
	DT_FOREACH_PROP_ELEM(node_id, code_partitions, TFM_TZ_PART_FROM_PH)                        \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, data_partitions),                                     \
		   (DT_FOREACH_PROP_ELEM(node_id, data_partitions, TFM_TZ_PART_FROM_PH)))

/* Emits "(size == ref) &&" per element which is chained with a trailing 1 below to
 * fold into a single boolean for BUILD_ASSERT.
 */
#define TFM_TZ_SIZE_EQ(node_id, prop, idx, ref_size)                                               \
	(DT_REG_SIZE(DT_PHANDLE_BY_IDX(node_id, prop, idx)) == (ref_size)) &&

#define TFM_TZ_CODE_SIZES_EQ(node_id, ref_size)                                                    \
	DT_FOREACH_PROP_ELEM_VARGS(node_id, code_partitions, TFM_TZ_SIZE_EQ, ref_size)

static const struct tfm_tz_part tfm_tz_secure_parts[] = {
	DT_FOREACH_STATUS_OKAY(nordic_tz_secure, TFM_TZ_PARTS_FROM_NODE)
};

static const struct tfm_tz_part tfm_tz_nonsecure_parts[] = {
	DT_FOREACH_STATUS_OKAY(nordic_tz_nonsecure, TFM_TZ_PARTS_FROM_NODE)
};

BUILD_ASSERT(
	(DT_FOREACH_STATUS_OKAY_VARGS(nordic_tz_secure, TFM_TZ_CODE_SIZES_EQ,
				      FLASH_S_PARTITION_SIZE) 1),
	"All entries of nordic,tz-secure code-partitions must have equal reg size "
	"(single-slot size; multi-slot layouts must match across all instances)");

BUILD_ASSERT(
	(DT_FOREACH_STATUS_OKAY_VARGS(nordic_tz_nonsecure, TFM_TZ_CODE_SIZES_EQ,
				      FLASH_NS_PARTITION_SIZE) 1),
	"All entries of nordic,tz-nonsecure code-partitions must have equal reg size "
	"(single-slot size; multi-slot layouts must match across all instances)");

/* Total fit isn't implied by the runtime contiguity check: a contiguous
 * block can still overflow the flash device.
 */
#if defined(FLASH_SECURE_TOTAL_SIZE) && defined(FLASH_NONSECURE_TOTAL_SIZE)
BUILD_ASSERT((SECURE_IMAGE_OFFSET + FLASH_SECURE_TOTAL_SIZE + FLASH_NONSECURE_TOTAL_SIZE) <=
		     FLASH_TOTAL_SIZE,
	     "TrustZone partitions extend past the end of the flash device");
#endif

static inline struct tfm_tz_summary tfm_tz_summarize(const struct tfm_tz_part *parts, size_t n)
{
	struct tfm_tz_summary s = { .min_start = UINT32_MAX, .max_end = 0, .sum_size = 0 };

	for (size_t i = 0; i < n; i++) {
		if (parts[i].start < s.min_start) {
			s.min_start = parts[i].start;
		}
		if (parts[i].end > s.max_end) {
			s.max_end = parts[i].end;
		}
		s.sum_size += parts[i].end - parts[i].start;
	}
	return s;
}

void tfm_tz_partition_check(void)
{
	const struct tfm_tz_summary secure =
		tfm_tz_summarize(tfm_tz_secure_parts, ARRAY_SIZE(tfm_tz_secure_parts));
	const struct tfm_tz_summary nonsecure =
		tfm_tz_summarize(tfm_tz_nonsecure_parts, ARRAY_SIZE(tfm_tz_nonsecure_parts));

	__ASSERT(secure.sum_size == (secure.max_end - secure.min_start),
		 "Secure TrustZone partitions (code + data) are not contiguous");
	__ASSERT(secure.min_start == SECURE_IMAGE_OFFSET,
		 "Secure partitions must start at SECURE_IMAGE_OFFSET (0x0)");
	__ASSERT(secure.sum_size == FLASH_SECURE_TOTAL_SIZE,
		 "FLASH_SECURE_TOTAL_SIZE macro does not match runtime sum");

	__ASSERT(nonsecure.sum_size == (nonsecure.max_end - nonsecure.min_start),
		 "Non-Secure TrustZone partitions (code + data) are not contiguous");
	__ASSERT(nonsecure.sum_size == FLASH_NONSECURE_TOTAL_SIZE,
		 "FLASH_NONSECURE_TOTAL_SIZE macro does not match runtime sum");

	__ASSERT(nonsecure.min_start == secure.max_end,
		 "Non-Secure partitions must begin exactly where Secure partitions end");
}

#else /* CONFIG_PARTITION_MANAGER_ENABLED */

void tfm_tz_partition_check(void)
{
}

#endif /* !CONFIG_PARTITION_MANAGER_ENABLED */
