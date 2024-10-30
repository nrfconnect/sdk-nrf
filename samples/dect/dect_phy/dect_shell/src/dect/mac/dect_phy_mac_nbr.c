/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include "desh_print.h"

#include "dect_phy_mac_nbr.h"

struct dect_phy_mac_nbr_info_list_item nbrs[DECT_PHY_MAC_MAX_NEIGBORS] = {0};

K_MUTEX_DEFINE(nbr_list_mutex);

struct dect_phy_mac_nbr_info_list_item *dect_phy_mac_nbr_info_get_by_long_rd_id(uint32_t long_rd_id)
{
	struct dect_phy_mac_nbr_info_list_item *found_nbr = NULL;

	k_mutex_lock(&nbr_list_mutex, K_FOREVER);
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbrs[i].reserved && nbrs[i].long_rd_id == long_rd_id) {
			found_nbr = &nbrs[i];
			goto exit;
		}
	}
exit:
	k_mutex_unlock(&nbr_list_mutex);
	return found_nbr;
}

static bool dect_phy_mac_nbr_info_store(struct dect_phy_mac_nbr_info_list_item list_item)
{
	bool stored = false;

	k_mutex_lock(&nbr_list_mutex, K_FOREVER);
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (!nbrs[i].reserved) {
			nbrs[i] = list_item;
			nbrs[i].reserved = true;
			stored = true;
			goto exit;
		}
	}
exit:
	k_mutex_unlock(&nbr_list_mutex);
	return stored;
}

bool dect_phy_mac_nbr_info_remove_by_long_rd_id(uint32_t long_rd_id)
{
	bool removed = false;

	k_mutex_lock(&nbr_list_mutex, K_FOREVER);
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbrs[i].long_rd_id == long_rd_id) {
			nbrs[i].reserved = false;
			removed = true;
			break;
		}
	}
	k_mutex_unlock(&nbr_list_mutex);
	return removed;
}

void dect_phy_mac_nbr_info_clear_all(void)
{
	k_mutex_lock(&nbr_list_mutex, K_FOREVER);
	memset(nbrs, 0, DECT_PHY_MAC_MAX_NEIGBORS * sizeof(struct dect_phy_mac_nbr_info_list_item));
	k_mutex_unlock(&nbr_list_mutex);
}

bool dect_phy_mac_nbr_info_store_n_update(uint64_t const *rcv_time, uint16_t channel,
					  uint32_t nw_id_24msb, uint8_t nw_id_8lsb,
					  uint32_t long_rd_id, uint16_t short_rd_id,
					  dect_phy_mac_cluster_beacon_t *beacon_msg,
					  dect_phy_mac_random_access_resource_ie_t *ra_ie)
{
	bool done = false;
	struct dect_phy_mac_nbr_info_list_item *nbr_ptr;

	nbr_ptr = dect_phy_mac_nbr_info_get_by_long_rd_id(long_rd_id);

	if (!nbr_ptr) {
		/* Insert as a new one if there is room */
		struct dect_phy_mac_nbr_info_list_item nbr_info = {
			.channel = channel,
			.time_rcvd_mdm_ticks = *rcv_time,
			.long_rd_id = long_rd_id,
			.short_rd_id = short_rd_id,
			.nw_id_24msb = nw_id_24msb,
			.nw_id_8lsb = nw_id_8lsb,
			.nw_id_32bit = ((nw_id_24msb << 8) | nw_id_8lsb),
			.beacon_msg = *beacon_msg,
			.ra_ie = *ra_ie,
		};

		if (!dect_phy_mac_nbr_info_store(nbr_info)) {
			desh_error("%s: cannot store scanning nbr result for long rd id %u",
				   (__func__), long_rd_id);
			done = false;
		} else {
			desh_print("Neighbor with long rd id %u (0x%08x), short rd id %u (0x%04x) "
				   "stored to nbr list.",
				   long_rd_id, long_rd_id, short_rd_id, short_rd_id);
		}
	} else {
		/* Already existing beacon: update */
		nbr_ptr->channel = channel;
		nbr_ptr->short_rd_id = short_rd_id;
		nbr_ptr->nw_id_24msb = nw_id_24msb;
		nbr_ptr->nw_id_8lsb = nw_id_8lsb;
		nbr_ptr->nw_id_32bit = ((nw_id_24msb << 8) | nw_id_8lsb);
		nbr_ptr->time_rcvd_mdm_ticks = *rcv_time;
		nbr_ptr->beacon_msg = *beacon_msg;
		nbr_ptr->ra_ie = *ra_ie; /* Note: storing only one RA IE */

		desh_print("Neighbor with long rd id %u (0x%08x), short rd id %u (0x%04x), "
			   "nw (24bit MSB: %u (0x%06x), 8bit LSB: %u (0x%02x))\n"
			   "  updated with time %llu to nbr list.",
			   long_rd_id, long_rd_id, short_rd_id, short_rd_id, nw_id_24msb,
			   nw_id_24msb, nw_id_8lsb, nw_id_8lsb, nbr_ptr->time_rcvd_mdm_ticks);
	}
	k_mutex_unlock(&nbr_list_mutex);
	return done;
}

void dect_phy_mac_nbr_status_print(void)
{
	k_mutex_lock(&nbr_list_mutex, K_FOREVER);

	desh_print("Neighbor list status:");
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbrs[i].reserved == true) {
			desh_print("  Neighbor %d:", i + 1);
			desh_print("   network ID (24bit MSB): %u (0x%06x)", nbrs[i].nw_id_24msb,
				   nbrs[i].nw_id_24msb);
			desh_print("   network ID (8bit LSB):  %u (0x%02x)", nbrs[i].nw_id_8lsb,
				   nbrs[i].nw_id_8lsb);
			desh_print("   network ID (32bit):     %u (0x%06x)", nbrs[i].nw_id_32bit,
				   nbrs[i].nw_id_32bit);
			desh_print("   long RD ID:             %u", nbrs[i].long_rd_id);
			desh_print("   short RD ID:            %u", nbrs[i].short_rd_id);
			desh_print("   channel:                %u", nbrs[i].channel);
			desh_print("   last seen time:         %llu\n",
				   nbrs[i].time_rcvd_mdm_ticks);
		}
	}

	k_mutex_unlock(&nbr_list_mutex);
}

bool dect_phy_mac_nbr_is_in_channel(uint16_t channel)
{
	bool return_value = false;

	k_mutex_lock(&nbr_list_mutex, K_FOREVER);
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbrs[i].reserved == true && channel == nbrs[i].channel) {
			return_value = true;
			break;
		}
	}

	k_mutex_unlock(&nbr_list_mutex);
	return return_value;
}
