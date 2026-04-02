/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SOC_NORDIC_NRF9251_H_
#define _SOC_NORDIC_NRF9251_H_

#include <soc_nrf_common.h>

#if defined(CONFIG_SOC_NRF9251_CPUAPP)
#define RAMBLOCK_CONTROL_BIT_ICACHE MEMCONF_POWER_CONTROL_MEM1_Pos
#define RAMBLOCK_CONTROL_BIT_DCACHE MEMCONF_POWER_CONTROL_MEM2_Pos
#define RAMBLOCK_POWER_ID           0
#define RAMBLOCK_CONTROL_OFF        0
#define RAMBLOCK_RET_MASK           (MEMCONF_POWER_RET_MEM0_Msk)
#define RAMBLOCK_RET_BIT_ICACHE     MEMCONF_POWER_RET_MEM1_Pos
#define RAMBLOCK_RET_BIT_DCACHE     MEMCONF_POWER_RET_MEM2_Pos
#endif

#endif /* _SOC_NORDIC_NRF9251_H_ */
