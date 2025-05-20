/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_PERF_H
#define DECT_PHY_PERF_H

#include <zephyr/kernel.h>

#define DECT_PHY_PERF_DURATION_INFINITE -1

/******************************************************************************/

int dect_phy_perf_cmd_handle(struct dect_phy_perf_params *params);
void dect_phy_perf_cmd_stop(void);

/******************************************************************************/

#endif /* DECT_PHY_PERF_H */
