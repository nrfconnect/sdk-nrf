/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_PING_H
#define DECT_PHY_PING_H

#include <zephyr/kernel.h>

/******************************************************************************/

int dect_phy_ping_cmd_handle(struct dect_phy_ping_params *params);
void dect_phy_ping_cmd_stop(void);

/******************************************************************************/

#endif /* DECT_PHY_PING_H */
