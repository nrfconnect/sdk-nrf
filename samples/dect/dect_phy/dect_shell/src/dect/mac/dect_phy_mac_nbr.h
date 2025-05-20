/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_NBR_H
#define DECT_PHY_MAC_NBR_H

#include <zephyr/kernel.h>

#include "dect_phy_mac_pdu.h"

#define DECT_PHY_MAC_MAX_NEIGBORS 16

/* Populated from cluster beacons & */
struct dect_phy_mac_nbr_info_list_item {
	bool reserved;

	uint16_t channel;

	uint32_t long_rd_id;
	uint16_t short_rd_id;

	uint32_t nw_id_24msb;
	uint8_t nw_id_8lsb;
	uint32_t nw_id_32bit;

	uint64_t time_rcvd_mdm_ticks;
	int64_t time_rcvd_shift_mdm_ticks;

	dect_phy_mac_cluster_beacon_t beacon_msg;
	dect_phy_mac_random_access_resource_ie_t ra_ie; /* Supporting only one RA IE */
};

struct dect_phy_mac_nbr_info_list_item *
dect_phy_mac_nbr_info_get_by_long_rd_id(uint32_t long_rd_id);

bool dect_phy_mac_nbr_info_remove_by_long_rd_id(uint32_t long_rd_id);
void dect_phy_mac_nbr_info_clear_all(void);

bool dect_phy_mac_nbr_info_store_n_update(uint64_t const *rcv_time, uint16_t channel,
					  uint32_t nw_id_24msb, uint8_t nw_id_8lsb,
					  uint32_t long_rd_id, uint16_t short_rd_id,
					  dect_phy_mac_cluster_beacon_t *beacon_msg,
					  dect_phy_mac_random_access_resource_ie_t *ra_ie,
					  bool print_update);

bool dect_phy_mac_nbr_is_in_channel(uint16_t channel);

void dect_phy_mac_nbr_status_print(void);

#endif /* DECT_PHY_MAC_NBR_H */
