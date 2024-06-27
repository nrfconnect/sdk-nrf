/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <fprotect.h>
#include <pm_config.h>
#include <zephyr/storage/flash_map.h>

#define PRIORITY_LEVEL 0 /* Locking should be performed as soon as possible */

#ifdef USE_PARTITION_MANAGER
#define AREA_ADDRESS PM_ADDRESS
#define AREA_SIZE PM_SIZE
#else
#define CODE_PARTITION DT_CHOSEN(zephyr_code_partition)
#define AREA_ADDRESS FIXED_PARTITION_NODE_OFFSET(CODE_PARTITION)
#define AREA_SIZE FIXED_PARTITION_NODE_SIZE(CODE_PARTITION)
#endif

BUILD_ASSERT(AREA_ADDRESS % CONFIG_FPROTECT_BLOCK_SIZE == 0,
	     "Unable to protect app image since its start address does not "
	     "align with the locking granularity of fprotect.");

BUILD_ASSERT(AREA_SIZE % CONFIG_FPROTECT_BLOCK_SIZE == 0,
	     "Unable to protect app image since its size does not align with "
	     "the locking granularity of fprotect.");

static int fprotect_self(void)
{
	int err = fprotect_area(AREA_ADDRESS, AREA_SIZE);
	if (err != 0) {
		__ASSERT(0,
			 "Unable to lock required area. Check address and "
			 "size against locking granularity.");
	}

	return 0;
}

SYS_INIT(fprotect_self, PRE_KERNEL_1, PRIORITY_LEVEL);
