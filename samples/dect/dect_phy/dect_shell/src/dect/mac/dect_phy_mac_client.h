/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_CLIENT_H
#define DECT_PHY_MAC_CLIENT_H

#include <zephyr/kernel.h>
#include "dect_phy_mac_common.h"

/******************************************************************************/

int dect_phy_mac_client_rach_tx_start(
	struct dect_phy_mac_nbr_info_list_item *target_nbr,
	struct dect_phy_mac_rach_tx_params *params);

void dect_mac_client_rach_tx_stop(void);

int dect_phy_mac_client_associate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				  struct dect_phy_mac_associate_params *params);

int dect_phy_mac_client_dissociate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				   struct dect_phy_mac_associate_params *params);

/******************************************************************************/

#endif /* DECT_PHY_MAC_CLIENT_H */
