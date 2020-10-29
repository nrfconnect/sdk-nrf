/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <init.h>
#include <pm_config.h>
#include <fprotect.h>

#define PRIORITY_LEVEL 0 /* Locking should be performed as soon as possible */

BUILD_ASSERT(PM_ADDRESS % CONFIG_FPROTECT_BLOCK_SIZE == 0,
	     "Unable to protect app image since its start address does not "
	     "align with the locking granularity of fprotect.");

BUILD_ASSERT(PM_SIZE % CONFIG_FPROTECT_BLOCK_SIZE == 0,
	     "Unable to protect app image since its size does not align with "
	     "the locking granularity of fprotect.");

static int fprotect_self(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	err = fprotect_area(PM_ADDRESS, PM_SIZE);
	if (err != 0) {
		__ASSERT(0,
			 "Unable to lock required area. Check address and "
			 "size against locking granularity.");
	}

	return 0;
}

SYS_INIT(fprotect_self, PRE_KERNEL_1, PRIORITY_LEVEL);
