/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DFU_LOCK_H_
#define _DFU_LOCK_H_

#include <sys/atomic.h>
#include <assert.h>

bool dfu_lock(const void *module_id);
void dfu_unlock(const void *module_id);

#endif /* _DFU_LOCK_H_ */
