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

#include "dect_common.h"
#include "dect_app_time.h"
#include "dect_phy_common.h"
#include "dect_phy_mac_cluster_beacon.h"
#include "dect_phy_mac_nbr_bg_scan.h"
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
					  dect_phy_mac_random_access_resource_ie_t *ra_ie,
					  bool print_update)
{
	bool done = true;
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
			.time_rcvd_shift_mdm_ticks = 0,
		};

		if (!dect_phy_mac_nbr_info_store(nbr_info)) {
			desh_error("%s: cannot store scanning nbr result for long rd id %u",
				   (__func__), long_rd_id);
			done = false;
		} else {
			desh_print("Neighbor with long rd id %u (0x%08x), short rd id %u (0x%04x), "
				   "channel %d, stored to nbr list.",
				   long_rd_id, long_rd_id, short_rd_id, short_rd_id, channel);
		}
	} else {
		/* Already existing beacon: update */
		int64_t time_shift_mdm_ticks =
			dect_phy_mac_cluster_beacon_rcv_time_shift_calculate(
				nbr_ptr->beacon_msg.cluster_beacon_period,
				nbr_ptr->time_rcvd_mdm_ticks,
				*rcv_time);

		if (channel) {
			nbr_ptr->channel = channel;
		}
		nbr_ptr->short_rd_id = short_rd_id;
		nbr_ptr->nw_id_24msb = nw_id_24msb;
		nbr_ptr->nw_id_8lsb = nw_id_8lsb;
		nbr_ptr->nw_id_32bit = ((nw_id_24msb << 8) | nw_id_8lsb);
		nbr_ptr->time_rcvd_mdm_ticks = *rcv_time;
		nbr_ptr->time_rcvd_shift_mdm_ticks = time_shift_mdm_ticks;
		dect_phy_mac_nbr_bg_scan_rcv_time_shift_update(
			long_rd_id, *rcv_time, time_shift_mdm_ticks);
		nbr_ptr->beacon_msg = *beacon_msg;
		nbr_ptr->ra_ie = *ra_ie; /* Note: storing only one RA IE */

		if (print_update) {
			desh_print("Neighbor with long rd id %u (0x%08x), short rd id %u (0x%04x), "
				"nw (24bit MSB: %u (0x%06x), 8bit LSB: %u (0x%02x)), channel %d\n"
				"  updated with time %llu (time shift %lld mdm ticks) to nbr list.",
				long_rd_id, long_rd_id, short_rd_id, short_rd_id, nw_id_24msb,
				nw_id_24msb, nw_id_8lsb, nw_id_8lsb, nbr_ptr->channel,
				nbr_ptr->time_rcvd_mdm_ticks, time_shift_mdm_ticks);
		}
	}
	k_mutex_unlock(&nbr_list_mutex);
	return done;
}

void dect_phy_mac_nbr_status_print(void)
{
	uint64_t time_now = dect_app_modem_time_now();

	k_mutex_lock(&nbr_list_mutex, K_FOREVER);
	desh_print("Neighbor list status:");
	for (int i = 0; i < DECT_PHY_MAC_MAX_NEIGBORS; i++) {
		if (nbrs[i].reserved == true) {
			int64_t time_from_last_received_ms = MODEM_TICKS_TO_MS(
					time_now - nbrs[i].time_rcvd_mdm_ticks);

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
			desh_print("   Last seen:              %d msecs ago",
				time_from_last_received_ms);
			dect_phy_mac_nbr_bg_scan_status_print_for_target_long_rd_id(
				nbrs[i].long_rd_id);
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
