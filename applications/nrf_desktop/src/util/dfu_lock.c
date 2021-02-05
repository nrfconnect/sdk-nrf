/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include "dfu_lock.h"
#define DFU_UNLOCKED	0

static atomic_t dfu_locked = ATOMIC_INIT(DFU_UNLOCKED);


bool dfu_lock(const void *module_id)
{
	return atomic_cas(&dfu_locked, DFU_UNLOCKED, (atomic_val_t)module_id);
}

void dfu_unlock(const void *module_id)
{
	bool success = atomic_cas(&dfu_locked, (atomic_val_t)module_id,
				  DFU_UNLOCKED);

	/* Module that have not locked dfu, should not try to unlock it. */
	__ASSERT_NO_MSG(success);
	ARG_UNUSED(success);
}
