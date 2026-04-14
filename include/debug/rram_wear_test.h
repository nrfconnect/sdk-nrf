/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RRAM_WEAR_TEST_H_
#define RRAM_WEAR_TEST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup rram_wear_test RRAM wear test
 *
 * Writes 0x00 then 0xFF over a flash partition (or a forced custom range on the
 * internal flash device) and verifies readback. Intended for bring-up / stress checks.
 * @{
 */

/**
 * @brief Flash partition (or custom range) parameters for wear test.
 *
 * For @c name @c "custom", @a addr_start and @a addr_end are byte offsets on the internal
 * flash device (@a addr_end exclusive). For any other name, offsets describe the partition
 * span from the flash map (see @ref rram_wear_test_partition_get); the verify path still
 * opens the area by name and ignores @a addr_* for the actual I/O.
 *
 * @a name may be an empty string when the partition has no DTS @c label (unnamed partition);
 * identification then relies on the flash map / node label.
 */
struct rram_wear_test_partition {
	const char *name;
	uint64_t addr_start;
	uint64_t addr_end;
};

/**
 * @brief Get the number of entries in the partition table.
 */
size_t rram_wear_test_partition_count(void);

/**
 * @brief Fill @a out with partition name and device byte range from the flash map.
 *
 * @param index Row index in <tt>[0, rram_wear_test_partition_count())</tt>.
 * @param[out] out Filled on success; @a name points at build-time string storage (possibly @c ""
 *        when the partition has no DTS @c label).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @a out is NULL.
 * @retval -ENOENT if @a index is out of range or the flash area cannot be opened.
 */
int rram_wear_test_partition_get(size_t index, struct rram_wear_test_partition *out);

/**
 * @brief Run write/readback verify on a named flash region or custom range.
 *
 * Uses the internal partition table (see @a index), or
 * a custom byte range on the internal flash device when @a addr_end is greater than @a addr_start.
 *
 * @param index Partition index from the array to test.
 * @param addr_start Optional start offset for a custom range (used if addr_end > addr_start).
 * @param addr_end Optional end offset (exclusive) for a custom range.
 * @param force Must be true for custom ranges or @c -EPERM is returned.
 *
 * @retval 0 on success, negative errno otherwise (@c -EIO on pattern mismatch).
 */
int rram_wear_test(size_t index, uint64_t addr_start, uint64_t addr_end, bool force);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* RRAM_WEAR_TEST_H_ */
