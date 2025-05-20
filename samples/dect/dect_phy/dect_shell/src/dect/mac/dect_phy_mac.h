/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_H
#define DECT_PHY_MAC_H

#include <zephyr/kernel.h>
#include "dect_common.h"
#include "dect_phy_common.h"

/******************************************************************************/

bool dect_phy_mac_handle(struct dect_phy_commmon_op_pdc_rcv_params *rcv_params);

bool dect_phy_mac_direct_pdc_handle(struct dect_phy_commmon_op_pdc_rcv_params *rcv_params);

/******************************************************************************/

#endif /* DECT_PHY_MAC_H */
