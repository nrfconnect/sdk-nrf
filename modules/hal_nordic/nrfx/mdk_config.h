/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MDK_CONFIG_NRF_H__
#define MDK_CONFIG_NRF_H__

/*
 * This is an extenstion of Zephyr-based mdk_config.h header, which
 * relates to products which are yet to be upstreamed.
 */

#include_next <mdk_config.h>

/*
 * These are mappings of Kconfig options describing build target
 * to the corresponding symbols used inside of nrfx-based MDK.
 */

#ifdef CONFIG_SOC_NRF54LC10A_CPUAPP
#ifndef NRF54LC10A_XXAA
#define NRF54LC10A_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LC10A_CPUFLPR
#ifndef NRF54LC10A_XXAA
#define NRF54LC10A_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LS05A_CPUAPP
#ifndef NRF54LS05A_XXAA
#define NRF54LS05A_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LS05A_DEVELOP_IN_NRF54LS05B
#ifndef DEVELOP_IN_NRF54LS05B
#define DEVELOP_IN_NRF54LS05B 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LS05B_CPUAPP
#ifndef NRF54LS05B_XXAA
#define NRF54LS05B_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LV10A_CPUAPP
#ifndef NRF54LV10A_XXAA
#define NRF54LV10A_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LV10A_CPUFLPR
#ifndef NRF54LV10A_XXAA
#define NRF54LV10A_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9251_CPUAPP
#ifndef NRF9220_XXAA
#define NRF9220_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9251_CPUPPR
#ifndef NRF9220_XXAA
#define NRF9220_XXAA 1
#endif
#ifndef NRF_PPR
#define NRF_PPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9251_CPUFLPR
#ifndef NRF9220_XXAA
#define NRF9220_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#endif /* MDK_CONFIG_NRF_H__ */
