/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_VALIDATION_INTERNAL_H__
#define BL_VALIDATION_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>


static bool within(uint32_t addr, uint32_t start, uint32_t end)
{
	if (start > end) {
		return false;
	}
	if (addr < start) {
		return false;
	}
	if (addr >= end) {
		return false;
	}
	return true;
}

static bool region_within(uint32_t inner_start, uint32_t inner_end,
			uint32_t start, uint32_t end)
{
	if (inner_start > inner_end) {
		return false;
	}
	if (!within(inner_start, start, end)) {
		return false;
	}
	if (!within(inner_end - 1, start, end)) {
		return false;
	}
	return true;
}

#ifdef __cplusplus
}
#endif

#endif /* BL_VALIDATION_INTERNAL_H__ */
