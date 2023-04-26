/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <debug/etb_trace.h>
#include <memfault/panics/platform/coredump.h>

#define ETB_BUFFER_VALID_MAGIC 0xDEADBEEF

static volatile uint32_t etb_buf[ETB_BUFFER_SIZE / 4];
static volatile uint32_t etb_buf_valid;

void memfault_ncs_etb_fault_handler(void)
{
	etb_trace_stop();
	etb_data_get(etb_buf, ARRAY_SIZE(etb_buf));

	etb_buf_valid = ETB_BUFFER_VALID_MAGIC;
}

size_t memfault_ncs_etb_get_regions(sMfltCoredumpRegion *regions, size_t num_regions)
{
	if (regions == NULL || num_regions < 1 || etb_buf_valid != ETB_BUFFER_VALID_MAGIC) {
		return 0;
	}

	// Region size should be
	regions[0] = (sMfltCoredumpRegion){
		.type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
		.region_start = (const void *)etb_buf,
		// Region size is in bytes, multiply by 4 as the ETB vars are in words
		.region_size = (ARRAY_SIZE(etb_buf) << 2),
	};
	return 1;
}
