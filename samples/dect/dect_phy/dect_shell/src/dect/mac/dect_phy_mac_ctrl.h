/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_CTRL_H
#define DECT_PHY_MAC_CTRL_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include "dect_phy_mac_common.h"

enum dect_phy_mac_ctrl_beacon_stop_cause {
	DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_USER_INITIATED,
	DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_LMS_BEACON_TX,
	DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_LMS_RACH,
};

/******************************************************************************/

int dect_phy_mac_ctrl_cluster_beacon_start(struct dect_phy_mac_beacon_start_params *params);
void dect_phy_mac_ctrl_cluster_beacon_stop(enum dect_phy_mac_ctrl_beacon_stop_cause cause);
int dect_phy_mac_ctrl_beacon_scan_start(struct dect_phy_mac_beacon_scan_params *params);
int dect_phy_mac_ctrl_rach_tx_start(struct dect_phy_mac_rach_tx_params *params);
int dect_phy_mac_ctrl_rach_tx_stop(void);
int dect_phy_mac_ctrl_associate(struct dect_phy_mac_associate_params *params);
int dect_phy_mac_ctrl_dissociate(struct dect_phy_mac_associate_params *params);

/******************************************************************************/

#endif /* DECT_PHY_MAC_CTRL_H */
