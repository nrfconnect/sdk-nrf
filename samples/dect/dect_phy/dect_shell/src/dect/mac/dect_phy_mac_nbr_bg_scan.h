/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_NBR_BG_SCAN_H
#define DECT_PHY_MAC_NBR_BG_SCAN_H

#include <zephyr/kernel.h>
#include "dect_phy_mac_nbr.h"

/******************************************************************************/

#define DECT_PHY_MAC_NBR_BG_SCAN_MAX_UNREACHABLE_TIME_MS (180 * 1000)

enum dect_phy_mac_nbr_bg_scan_op_completed_cause {
	DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_UNKNOWN,
	DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_USER_INITIATED,
	DECT_PHY_MAC_NBR_BG_SCAN_OP_COMPLETED_CAUSE_UNREACHABLE,
};

struct dect_phy_mac_nbr_bg_scan_op_completed_info {
	enum dect_phy_mac_nbr_bg_scan_op_completed_cause cause;
	uint32_t phy_op_handle;
	uint32_t target_long_rd_id;
};

typedef void (*dect_phy_mac_nbr_scan_completed_cb_t)(
	struct dect_phy_mac_nbr_bg_scan_op_completed_info *info);

struct dect_phy_mac_nbr_bg_scan_params {
	uint32_t phy_op_handle;

	struct dect_phy_mac_nbr_info_list_item *target_nbr;
	uint32_t target_long_rd_id;

	dect_phy_mac_nbr_scan_completed_cb_t cb_op_completed;
};

int dect_phy_mac_nbr_bg_scan_start(struct dect_phy_mac_nbr_bg_scan_params *params);
int dect_phy_mac_nbr_bg_scan_stop(uint32_t op_handle);

void dect_phy_mac_nbr_bg_scan_rcv_time_shift_update(uint32_t nbr_long_rd_id, uint64_t time_rcvd,
						    int64_t time_shift_mdm_ticks);

void dect_phy_mac_nbr_bg_scan_status_print_for_target_long_rd_id(uint32_t long_rd_id);

/******************************************************************************/

#endif /* DECT_PHY_MAC_NBR_BG_SCAN_H */
