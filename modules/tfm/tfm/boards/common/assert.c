/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <pm_config.h>
#include "autoconf.h"
#include "region_defs.h"


#define IS_ALIGNED_POW2(value, align) (((value) & ((align) - 1)) == 0)

/* Making sure the borders between secure and non-secure regions are aligned with the SPU regions */

#if !(IS_ALIGNED_POW2(PM_TFM_NONSECURE_ADDRESS, CONFIG_NRF_SPU_FLASH_REGION_SIZE))
#error "TF-M non-secure address start is not aligned on SPU region size"
#endif

#if !(IS_ALIGNED_POW2(PM_SRAM_NONSECURE_ADDRESS, SPU_SRAM_REGION_SIZE))
#error "SRAM non-secure address size is not aligned on SPU region size"
#endif
