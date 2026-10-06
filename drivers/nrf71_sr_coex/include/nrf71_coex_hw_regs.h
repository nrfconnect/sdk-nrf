/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Controller (COEXC) hardware register definitions.
 */

#ifndef NRF71_COEX_HW_REGS_H__
#define NRF71_COEX_HW_REGS_H__

#include <stdint.h>

/** COEXC clock frequency used for turnaround register programming (MHz). */
#define COEXC_CLK_FREQ_MHZ 16U

/** Total number of COEXC clients (Wi-Fi, SR, external). */
#define COEXC_NUM_CLIENTS 8U
/** Number of CCMALLOW modes per client. */
#define COEXC_NUM_MODES 4U

/** Global-domain COEXC CCMALLOW register addresses. */
#define ABS_COEXC_CCMALLOW_0_MODE_0 0x400F1400UL
#define ABS_COEXC_CCMALLOW_0_MODE_1 0x400F1404UL
#define ABS_COEXC_CCMALLOW_1_MODE_0 0x400F1420UL

/** Global-domain addresses for COEXC CCCONF and TURNAROUND blocks. */
#define ABS_COEXC_CCCONF_0     0x400F1600UL
#define ABS_COEXC_TURNAROUND_0 0x400F1640UL

#define COEXC_OFFSET_BETWEEN_MODES   (ABS_COEXC_CCMALLOW_0_MODE_1 - ABS_COEXC_CCMALLOW_0_MODE_0)
#define COEXC_OFFSET_BETWEEN_CLIENTS (ABS_COEXC_CCMALLOW_1_MODE_0 - ABS_COEXC_CCMALLOW_0_MODE_0)

#define COEXC_CCCONF_0_PRIORITY_SHIFT 16U
#define COEXC_CCCONF_0_MODE_SHIFT      0U

/** Antenna configuration used when programming CCMALLOW tables. */
enum coex_antenna_cfg_type {
	COEX_SHARED_ANT_CFG = 0,
	COEX_SEPARATE_ANT_CFG = 1,
};

#endif /* NRF71_COEX_HW_REGS_H__ */
