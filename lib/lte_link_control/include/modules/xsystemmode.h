/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef XSYSTEMMODE_H__
#define XSYSTEMMODE_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Set the modem's system mode and LTE preference. */
int xsystemmode_mode_set(enum lte_lc_system_mode mode,
			 enum lte_lc_system_mode_preference preference);

/* Get the modem's system mode and LTE preference. */
int xsystemmode_mode_get(enum lte_lc_system_mode *mode,
			 enum lte_lc_system_mode_preference *preference);

#ifdef __cplusplus
}
#endif

#endif /* XSYSTEMMODE_H__ */
