/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __MMIO_DEFS_H__
#define __MMIO_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tfm_peripherals_def.h"
#include "tfm_peripherals_config.h"
#include "handle_attr.h"

/* Allowed named MMIO of this platform */
const uintptr_t partition_named_mmio_list[] = {
	// TODO: NCSDK-22597: Populate this list
};

#ifdef __cplusplus
}
#endif

#endif /* __MMIO_DEFS_H__ */
