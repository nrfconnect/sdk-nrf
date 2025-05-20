/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_CLIENT_H
#define DECT_PHY_MAC_CLIENT_H

#include <zephyr/kernel.h>
#include "dect_phy_mac_common.h"

#define DECT_PHY_MAC_CLIENT_ASSOCIATION_RESP_WAIT_TIME_SEC (3)

/******************************************************************************/

int dect_phy_mac_client_rach_tx_start(
	struct dect_phy_mac_nbr_info_list_item *target_nbr,
	struct dect_phy_mac_rach_tx_params *params);

void dect_mac_client_rach_tx_stop(void);

int dect_phy_mac_client_associate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				  struct dect_phy_mac_associate_params *params);

int dect_phy_mac_client_dissociate(struct dect_phy_mac_nbr_info_list_item *target_nbr,
				   struct dect_phy_mac_associate_params *params);

void dect_phy_mac_client_associate_resp_handle(
	dect_phy_mac_common_header_t *common_header,
	dect_phy_mac_association_resp_t *association_resp);

void dect_phy_mac_client_status_print(void);

/******************************************************************************/

bool dect_phy_mac_client_associated_by_target_short_rd_id(uint16_t target_short_rd_id);

#endif /* DECT_PHY_MAC_CLIENT_H */
